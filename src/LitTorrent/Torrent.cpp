#include "LitTorrent/Torrent.h"

#include "../Utils/SHA1.h"
#include "FileItem.h"
#include "LitTorrent/BEncoding.h"
#include "LitTorrent/Tracker.h"
#include "Logger.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <ios>
#include <locale>
#include <memory>

namespace LitTorrent {

namespace {
namespace Internal {
static ByteArray encodeUTF8String(const std::string &str) {
  return ByteArray(str.begin(), str.end());
}

static std::string decodeUTF8String(const ByteArray &bytes) {
  return std::string(bytes.begin(), bytes.end());
}
} // namespace Internal
} // namespace

// Constructor
Torrent::Torrent(std::string name, std::string location,
                 std::vector<FileItem> files, std::vector<std::string> trackers,
                 int pieceSize, std::vector<std::string> pieceHashes,
                 int blockSize, bool isPrivate)
    : name_(name), isPrivate_(isPrivate), downloadDirectory_(location),
      comment_(""), createdBy_(""), creationDate_(0), encoding_(""),
      blockSize_(blockSize), pieceSize_(pieceSize), files_(files) {

  // Init trackers
  for (auto &trackerUrl : trackers) {
    auto tracker = std::make_unique<Tracker>(trackerUrl);
    // tracker->addPeerListUpdatedHandler();
    trackers_.push_back(std::move(tracker));
  }

  int pieceCount = ceil(getTotalSize() * 1.0 / pieceSize);

  pieceHashes_.resize(pieceCount);
  isPieceVerified_.resize(pieceCount);
  isBlockAcquired_.resize(pieceCount);

  for (int i = 0; i < pieceCount; i++) {
    isBlockAcquired_[i].resize(getBlockCount(i));
  }

  if (!pieceHashes.empty()) {
    pieceHashes_ = pieceHashes;
  } else {
    for (int i = 0; i < pieceCount; i++) {
      pieceHashes[i] = getHash(i);
    }
  }

  infoHash_.resize(20);
}

// Destructor
Torrent::~Torrent() {}

int Torrent::getPieceCount() const { return pieceHashes_.size(); }

int Torrent::getPieceSize(int pieceIdx) const {
  int pieceCount = getPieceCount();
  assert(pieceIdx >= pieceCount);
  if (pieceIdx != pieceCount - 1) {
    return pieceSize_;
  }
  int remainder = getTotalSize() % pieceSize_;
  return remainder == 0 ? pieceSize_ : remainder;
}

int Torrent::getBlockSize(int pieceIdx, int blockIdx) const {
  int blockCount = getBlockCount(pieceIdx);
  assert(blockIdx >= blockCount);
  if (blockIdx != blockCount - 1) {
    return blockSize_;
  }
  int remainder = getPieceSize(pieceIdx) % blockSize_;
  return remainder == 0 ? blockSize_ : remainder;
}

int Torrent::getBlockCount(int pieceIdx) const {
  return ceil(getPieceSize(pieceIdx) * 1.0 / blockSize_);
}

std::string Torrent::readPiece(int pieceIdx) const {
  return read(pieceSize_ * pieceIdx, getPieceSize(pieceIdx));
}

std::string Torrent::readBlock(int pieceIdx, int offset, int length) {
  return read(pieceSize_ * pieceIdx + offset, length);
}

void Torrent::writeBlock(int pieceIdx, int blockIdx, std::string bytes) {
  write(pieceSize_ * pieceIdx + blockIdx * blockSize_, bytes);
  isBlockAcquired_[pieceIdx][blockIdx] = true;
  verify(pieceIdx);
}

std::string Torrent::getHash(int pieceIdx) const {
  std::string bytes = readPiece(pieceIdx);
  return SHA1::computeHash(bytes);
}

std::string Torrent::read(int start, int count) const {
  std::string bytes(count, '\0');
  int end = start + count;

  for (auto &file : files_) {
    int fileStart = file.getOffset();
    int fileEnd = file.getOffset() + file.getSize();

    // Check if this file overlaps with the requested range
    if (fileStart < end && fileEnd > start) {
      // Calculate offset within the file to start reading
      int fstart = std::max(0, start - fileStart);

      // Calculate how many bytes to read from this file
      int readEnd = std::min(end, fileEnd);
      int flength = readEnd - std::max(start, fileStart);

      // Calculate offset in the output buffer
      int bstart = std::max(0, fileStart - start);

      std::fstream f(file.getFilePath(),
                     std::ios_base::in | std::ios_base::binary);
      if (f.is_open()) {
        f.seekg(fstart);
        f.read(&bytes[bstart], flength);
        f.close();
      }
    }
  }
  return bytes;
}

void Torrent::write(int start, const std::string &buffer) const {
  int end = start + buffer.size();
  for (auto &file : files_) {
    int fileStart = file.getOffset();
    int fileEnd = file.getOffset() + file.getSize();

    // Check if this file overlaps with the requested range
    if (fileStart < end && fileEnd > start) {
      int fstart = std::max(0, start - fileStart);
      int writeEnd = std::min(end, fileEnd);
      int writeLen = writeEnd - std::max(fileStart, start);

      // Calculate offset in the input buffer
      int bstart = std::max(0, fileStart - start);

      std::fstream f(file.getFilePath(), std::ios_base::in |
                                             std::ios_base::out |
                                             std::ios_base::binary);
      if (f.is_open()) {
        f.seekp(fstart);
        f.write(&buffer[bstart], writeLen);
        f.close();
      }
    }
  }
}

void Torrent::setPieceVerifiedCallback(PieceVerifiedCallback callback) {
  this->pieceVerifiedCb_ = std::move(callback);
}

size_t Torrent::getTotalSize() const {
  int totalSize = 0;
  for (auto &file : files_) {
    totalSize += file.getSize();
  }
  return totalSize;
}

void Torrent::verify(int pieceIdx) {
  std::string pieceHash = getHash(pieceIdx);
  if (pieceHash == pieceHashes_[pieceIdx]) {
    isPieceVerified_[pieceIdx] = true;
    for (int i = 0; i < isBlockAcquired_[pieceIdx].size(); i++) {
      isBlockAcquired_[pieceIdx][i] = true;
    }
    if (pieceVerifiedCb_) {
      pieceVerifiedCb_(pieceIdx);
    }
    return;
  }
  LOG_INFO("Hash of piece %d does not match", pieceIdx);
  isPieceVerified_[pieceIdx] = false;
  if (std::all_of(isBlockAcquired_[pieceIdx].begin(),
                  isBlockAcquired_[pieceIdx].end(), [](bool x) { return x; })) {
    std::fill(isBlockAcquired_[pieceIdx].begin(),
              isBlockAcquired_[pieceIdx].end(), false);
  }
}

// Static method
TorrentPtr Torrent::BEncodedObjToTorrent(BEncodedValuePtr object) {
  if (object->GetType() != BEncodedValue::Type::Dictionary) {
    throw std::runtime_error("not a torrent file");
  }

  BEncodedDict obj = object->GetDictionary();

  // Extract trackers
  std::vector<std::string> trackers;
  if (obj.find("announce") != obj.end()) {
    auto announceValue = obj["announce"];
    if (announceValue->GetType() == BEncodedValue::Type::ByteArray) {
      trackers.push_back(
          Internal::decodeUTF8String(announceValue->GetByteArray()));
    } else if (announceValue->GetType() == BEncodedValue::Type::List) {
      auto announceList = announceValue->GetList();
      for (const auto &item : announceList) {
        trackers.push_back(Internal::decodeUTF8String(item->GetByteArray()));
      }
    }
  }

  // Extract info dictionary
  if (obj.find("info") == obj.end()) {
    throw std::runtime_error("Missing info section");
  }

  auto infoValue = obj["info"];
  if (infoValue->GetType() != BEncodedValue::Type::Dictionary) {
    throw std::runtime_error("Invalid info section");
  }

  BEncodedDict info = infoValue->GetDictionary();

  // Extract files
  std::vector<FileItem> files;

  if (info.find("name") != info.end() && info.find("length") != info.end()) {
    // Single file mode
    std::string path = Internal::decodeUTF8String(info["name"]->GetByteArray());
    int64_t size = info["length"]->GetNumber();

    files.push_back(FileItem(path, size, 0));
  } else if (info.find("files") != info.end()) {
    // Multi-file mode
    auto filesList = info["files"]->GetList();
    int64_t running = 0;

    for (const auto &item : filesList) {
      if (item->GetType() != BEncodedValue::Type::Dictionary) {
        throw std::runtime_error("error: incorrect file specification");
      }

      BEncodedDict fileDict = item->GetDictionary();

      if (fileDict.find("path") == fileDict.end() ||
          fileDict.find("length") == fileDict.end()) {
        throw std::runtime_error("error: incorrect file specification");
      }

      // Reconstruct path from list
      auto pathList = fileDict["path"]->GetList();
      std::string path;
      for (size_t i = 0; i < pathList.size(); ++i) {
        if (i > 0) {
          path += std::filesystem::path::preferred_separator;
        }
        path += Internal::decodeUTF8String(pathList[i]->GetByteArray());
      }

      int64_t size = fileDict["length"]->GetNumber();

      files.push_back(FileItem(path, size, running));
      running += size;
    }
  } else {
    throw std::runtime_error("error: no files specified in torrent");
  }

  // Extract piece length
  if (info.find("piece length") == info.end()) {
    throw std::runtime_error("Missing piece length");
  }
  int pieceSize = static_cast<int>(info["piece length"]->GetNumber());

  // Extract pieces (hashes)
  if (info.find("pieces") == info.end()) {
    throw std::runtime_error("Missing pieces");
  }
  ByteArray piecesBytes = info["pieces"]->GetByteArray();

  // Split into individual 20-byte hashes
  std::vector<std::string> pieceHashes;
  for (size_t i = 0; i < piecesBytes.size(); i += 20) {
    pieceHashes.push_back(
        std::string(piecesBytes.begin() + i, piecesBytes.begin() + i + 20));
  }

  // Extract private flag
  bool isPrivate = false;
  if (info.find("private") != info.end()) {
    isPrivate = info["private"]->GetNumber() == 1;
  }

  // Extract name for torrent (from info dict)
  std::string name = Internal::decodeUTF8String(info["name"]->GetByteArray());

  // Create torrent (you'll need to provide downloadPath)
  std::string downloadPath = ""; // You may need to pass this as parameter
  auto torrent =
      std::make_shared<Torrent>(name, downloadPath, files, trackers, pieceSize,
                                pieceHashes, 16384, isPrivate);

  // Set optional fields
  if (obj.find("comment") != obj.end()) {
    torrent->comment_ = 
        Internal::decodeUTF8String(obj["comment"]->GetByteArray());
  }

  if (obj.find("created by") != obj.end()) {
    torrent->createdBy_ = 
        Internal::decodeUTF8String(obj["created by"]->GetByteArray());
  }

  if (obj.find("creation date") != obj.end()) {
    torrent->creationDate_ = obj["creation date"]->GetNumber();
  }

  if (obj.find("encoding") != obj.end()) {
    torrent->encoding_ =
        Internal::decodeUTF8String(obj["encoding"]->GetByteArray());
  }

  return torrent;
}

// Convert Torrent info dictionary to BEncodedValue
BEncodedValuePtr Torrent::torrentInfoToBEncodedObj(TorrentPtr torrent) {
  BEncodedDict dict;

  // piece length
  dict["piece length"] = BEncodedValue::CreateNumber(torrent->getPieceSize(0));

  // pieces - concatenate all piece hashes
  ByteArray pieces;
  const auto &pieceHashes = torrent->pieceHashes_;
  for (const auto &hash : pieceHashes) {
    pieces.insert(pieces.end(), hash.begin(), hash.end());
  }
  dict["pieces"] = BEncodedValue::CreateByteArray(pieces);

  // private (optional)
  auto isPrivate = torrent->isPrivate_;
  if (isPrivate.has_value()) {
    dict["private"] = BEncodedValue::CreateNumber(isPrivate.value() ? 1 : 0);
  }

  // files
  const auto &files = torrent->files_;
  if (files.size() == 1) {
    // Single file mode
    dict["name"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(files[0].getFilePath()));
    dict["length"] = BEncodedValue::CreateNumber(files[0].getSize());
  } else {
    // Multi-file mode
    BEncodedList filesList;

    for (const auto &f : files) {
      BEncodedDict fileDict;

      // Split path by directory separator and create list
      BEncodedList pathList;
      std::string path = f.getFilePath();
      size_t pos = 0;
      size_t found;

      while ((found = path.find(std::filesystem::path::preferred_separator,
                                pos)) != std::string::npos) {
        if (found > pos) {
          std::string segment = path.substr(pos, found - pos);
          pathList.push_back(BEncodedValue::CreateByteArray(
              Internal::encodeUTF8String(segment)));
        }
        pos = found + 1;
      }
      if (pos < path.length()) {
        pathList.push_back(BEncodedValue::CreateByteArray(
            Internal::encodeUTF8String(path.substr(pos))));
      }

      fileDict["path"] = BEncodedValue::CreateList(pathList);
      fileDict["length"] = BEncodedValue::CreateNumber(f.getSize());
      filesList.push_back(BEncodedValue::CreateDictionary(fileDict));
    }

    dict["files"] = BEncodedValue::CreateList(filesList);

    // Remove trailing separator from directory name
    std::string fileDir = torrent->downloadDirectory_;
    if (!fileDir.empty() &&
        fileDir.back() == std::filesystem::path::preferred_separator) {
      fileDir = fileDir.substr(0, fileDir.length() - 1);
    }
    dict["name"] =
        BEncodedValue::CreateByteArray(Internal::encodeUTF8String(fileDir));
  }

  return BEncodedValue::CreateDictionary(dict);
}

BEncodedValuePtr Torrent::TorrentToBEncodedObj(TorrentPtr torrent) {
  BEncodedDict dict;

  // dict["announce"] =
  // BEncodedValue::CreateByteArray(EncodeUTF8String(trackerAddress));

  // comment
  if (!torrent->comment_.empty()) {
    dict["comment"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->comment_));
  }

  // created by
  if (!torrent->createdBy_.empty()) {
    dict["created by"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->createdBy_));
  }

  // creation date
  dict["creation date"] = BEncodedValue::CreateNumber(torrent->creationDate_);

  // encoding
  if (!torrent->encoding_.empty()) {
    dict["encoding"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->encoding_));
  }

  // info
  dict["info"] = torrentInfoToBEncodedObj(torrent);

  return BEncodedValue::CreateDictionary(dict);
}

TorrentPtr Torrent::loadFromFile(std::filesystem::path filePath,
                                 std::filesystem::path downloadDir) {
  auto object = BEncoding::DecodeFile(filePath);
  return BEncodedObjToTorrent(object);
}

void Torrent::saveToFile(TorrentPtr torrent) {
  auto object = TorrentToBEncodedObj(torrent);
  return BEncoding::EncodeToFile(object, torrent->name_ + ".torrent");
}
//
} // namespace LitTorrent
