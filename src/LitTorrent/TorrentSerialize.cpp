#include "LitTorrent/Torrent.h"
#include "LitTorrent/Tracker.h"
#include "../Utils/SHA1.h"
#include "Error.h"
#include "FileItem.h"
#include "LitTorrent/BEncoding.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>

namespace LitTorrent {

namespace {
namespace Internal {
static ByteArray encodeUTF8String(const std::string &str) {
  return ByteArray(str.begin(), str.end());
}

static std::string decodeUTF8String(const ByteArray &bytes) {
  return std::string(bytes.begin(), bytes.end());
}

static std::vector<FileItem> collectFileWithinDir(const fs::path& path){
    std::vector<FileItem> files;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                auto filepath = entry.path();
                files.push_back(FileItem(filepath, fs::file_size(filepath)));
            }
        }
    } catch (const fs::filesystem_error &e) {
      LOG_ERROR("Error: %s", e.what());
    }

    return files;
}
} // namespace Internal
} // namespace

// Static method to load from BEncoded object
TorrentPtr Torrent::fromBEncodedObj(BEncodedValuePtr object,
                                    const std::string &downloadPath) {
  if (!object || object->GetType() != BEncodedValue::Type::Dictionary) {
    throw TorrentException(ErrorCode::InvalidTorrentFile,
                          "Root element is not a dictionary");
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
        if (item->GetType() == BEncodedValue::Type::ByteArray) {
          trackers.push_back(Internal::decodeUTF8String(item->GetByteArray()));
        }
      }
    }
  }

  if (trackers.empty()) {
    throw TorrentException(ErrorCode::MissingTrackers,
                          "No trackers found in torrent file");
  }

  // Extract info dictionary
  if (obj.find("info") == obj.end()) {
    throw TorrentException(ErrorCode::MissingInfoSection,
                          "Missing 'info' section in torrent file");
  }

  auto infoValue = obj["info"];
  if (infoValue->GetType() != BEncodedValue::Type::Dictionary) {
    throw TorrentException(ErrorCode::InvalidTorrentFile,
                          "'info' section is not a dictionary");
  }

  BEncodedDict info = infoValue->GetDictionary();

  // Extract files
  std::vector<FileItem> files;
  std::string torrentName;

  if (info.find("name") != info.end()) {
    torrentName = Internal::decodeUTF8String(info["name"]->GetByteArray());
  }

  if (info.find("length") != info.end()) {
    // Single file mode
    std::string path = downloadPath;
    if (!path.empty() &&
        path.back() != fs::path::preferred_separator) {
      path += fs::path::preferred_separator;
    }
    path += torrentName;

    int64_t size = info["length"]->GetNumber();
    files.push_back(FileItem(path, size, 0));
  } else if (info.find("files") != info.end()) {
    // Multi-file mode
    auto filesList = info["files"]->GetList();
    int64_t running = 0;

    std::string baseDir = downloadPath;
    if (!baseDir.empty() &&
        baseDir.back() != fs::path::preferred_separator) {
      baseDir += fs::path::preferred_separator;
    }
    baseDir += torrentName;

    for (const auto &item : filesList) {
      if (item->GetType() != BEncodedValue::Type::Dictionary) {
        throw TorrentException(ErrorCode::InvalidTorrentFile,
                              "File entry is not a dictionary");
      }

      BEncodedDict fileDict = item->GetDictionary();

      if (fileDict.find("path") == fileDict.end() ||
          fileDict.find("length") == fileDict.end()) {
        throw TorrentException(ErrorCode::InvalidTorrentFile,
                              "File entry missing 'path' or 'length'");
      }

      // Reconstruct path from list
      auto pathList = fileDict["path"]->GetList();
      std::string path = baseDir;
      for (size_t i = 0; i < pathList.size(); ++i) {
        if (i > 0 || !baseDir.empty()) {
          path += fs::path::preferred_separator;
        }
        path += Internal::decodeUTF8String(pathList[i]->GetByteArray());
      }

      int64_t size = fileDict["length"]->GetNumber();
      files.push_back(FileItem(path, size, running));
      running += size;
    }
  } else {
    throw TorrentException(ErrorCode::InvalidTorrentFile,
                          "No files specified (missing 'length' or 'files')");
  }

  // Extract piece length
  if (info.find("piece length") == info.end()) {
    throw TorrentException(ErrorCode::InvalidTorrentFile,
                          "Missing 'piece length'");
  }
  int pieceSize = static_cast<int>(info["piece length"]->GetNumber());

  // Extract pieces (hashes)
  if (info.find("pieces") == info.end()) {
    throw TorrentException(ErrorCode::InvalidTorrentFile, "Missing 'pieces'");
  }
  ByteArray piecesBytes = info["pieces"]->GetByteArray();

  // Split into individual 20-byte hashes
  std::vector<Hash> pieceHashes;
  for (size_t i = 0; i < piecesBytes.size(); i += 20) {
    if (i + 20 <= piecesBytes.size()) {
      Hash hash;
      std::copy(piecesBytes.begin() + i, piecesBytes.begin() + i + 20,
                hash.begin());
      pieceHashes.push_back(hash);
    }
  }

  // Extract private flag
  bool isPrivate = false;
  if (info.find("private") != info.end()) {
    isPrivate = info["private"]->GetNumber() == 1;
  }

  // Create torrent
  auto torrent = std::make_shared<Torrent>(torrentName, downloadPath, files,
                                           trackers, pieceSize, pieceHashes,
                                           16384, isPrivate);

  // Set optional metadata fields
  if (obj.find("comment") != obj.end()) {
    torrent->metadata_.comment =
        Internal::decodeUTF8String(obj["comment"]->GetByteArray());
  }

  if (obj.find("created by") != obj.end()) {
    torrent->metadata_.createdBy =
        Internal::decodeUTF8String(obj["created by"]->GetByteArray());
  }

  if (obj.find("creation date") != obj.end()) {
    torrent->metadata_.creationDate = obj["creation date"]->GetNumber();
  }

  if (obj.find("encoding") != obj.end()) {
    torrent->metadata_.encoding =
        Internal::decodeUTF8String(obj["encoding"]->GetByteArray());
  }

  // Compute info hash
  auto infoEncoded = Internal::decodeUTF8String(BEncoding::Encode(infoValue));
  std::string infoHashStr = SHA1::computeHash(infoEncoded);
  torrent->metadata_.infoHash = BytesToHash(infoHashStr);

  return torrent;
}

// Convert Torrent info dictionary to BEncodedValue
BEncodedValuePtr Torrent::torrentInfoToBEncodedObj(TorrentPtr torrent) {
  if (!torrent) {
    throw TorrentException(ErrorCode::InvalidParameter,
                          "Torrent pointer is null");
  }

  BEncodedDict dict;

  // piece length
  dict["piece length"] =
      BEncodedValue::CreateNumber(torrent->metadata_.pieceSize);

  // pieces - concatenate all piece hashes
  ByteArray pieces;
  for (const auto &hash : torrent->metadata_.pieceHashes) {
    pieces.insert(pieces.end(), hash.begin(), hash.end());
  }
  dict["pieces"] = BEncodedValue::CreateByteArray(pieces);

  // private (optional)
  if (torrent->metadata_.isPrivate.has_value()) {
    dict["private"] = BEncodedValue::CreateNumber(
        torrent->metadata_.isPrivate.value() ? 1 : 0);
  }

  // files
  const auto &files = torrent->files_;
  if (files.size() == 1) {
    // Single file mode
    fs::path filePath = files[0].getFilePath();
    dict["name"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(filePath.filename().string()));
    dict["length"] = BEncodedValue::CreateNumber(files[0].getSize());
  } else {
    // Multi-file mode
    BEncodedList filesList;

    for (const auto &f : files) {
      BEncodedDict fileDict;

      // Split path by directory separator and create list
      BEncodedList pathList;
      fs::path path = f.getFilePath();

      // Remove the base directory from path
      fs::path relativePath = path.lexically_relative(
          fs::path(torrent->downloadDirectory_) /
          torrent->metadata_.name);

      for (const auto &component : relativePath) {
        if (!component.empty() && component != ".") {
          pathList.push_back(BEncodedValue::CreateByteArray(
              Internal::encodeUTF8String(component.string())));
        }
      }

      fileDict["path"] = BEncodedValue::CreateList(pathList);
      fileDict["length"] = BEncodedValue::CreateNumber(f.getSize());
      filesList.push_back(BEncodedValue::CreateDictionary(fileDict));
    }

    dict["files"] = BEncodedValue::CreateList(filesList);
    dict["name"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->metadata_.name));
  }

  return BEncodedValue::CreateDictionary(dict);
}

BEncodedValuePtr Torrent::toBEncodedObj(TorrentPtr torrent) {
  if (!torrent) {
    throw TorrentException(ErrorCode::InvalidParameter,
                          "Torrent pointer is null");
  }

  BEncodedDict dict;

  // announce
  if (torrent->trackers_.size() == 1) {
    dict["announce"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->trackers_.front()->getAddress()));
  } else if (torrent->trackers_.size() > 1) {
    BEncodedList trackerList;
    for (const auto &tracker : torrent->trackers_) {
      trackerList.push_back(BEncodedValue::CreateByteArray(
          Internal::encodeUTF8String(tracker->getAddress())));
    }
    dict["announce-list"] = BEncodedValue::CreateList(trackerList);
    // Also set first tracker as main announce
    dict["announce"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->trackers_.front()->getAddress()));
  }

  // Optional metadata
  if (!torrent->metadata_.comment.empty()) {
    dict["comment"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->metadata_.comment));
  }

  if (!torrent->metadata_.createdBy.empty()) {
    dict["created by"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->metadata_.createdBy));
  }

  if (torrent->metadata_.creationDate != 0) {
    dict["creation date"] =
        BEncodedValue::CreateNumber(torrent->metadata_.creationDate);
  }

  if (!torrent->metadata_.encoding.empty()) {
    dict["encoding"] = BEncodedValue::CreateByteArray(
        Internal::encodeUTF8String(torrent->metadata_.encoding));
  }

  // info
  dict["info"] = torrentInfoToBEncodedObj(torrent);

  return BEncodedValue::CreateDictionary(dict);
}

TorrentPtr Torrent::loadFromFile(fs::path filePath,
                                 fs::path downloadDir) {
  try {
    auto object = BEncoding::DecodeFile(filePath);
    return fromBEncodedObj(object, downloadDir.string());
  } catch (const TorrentException &) {
    throw; // Re-throw our exceptions
  } catch (const std::exception &e) {
    throw TorrentException(ErrorCode::InvalidTorrentFile,
                          std::string("Failed to decode torrent file: ") +
                              e.what());
  }
}

void Torrent::saveToFile(TorrentPtr torrent,
                         fs::path outputPath) {
  if (!torrent) {
    throw TorrentException(ErrorCode::InvalidParameter,
                          "Torrent pointer is null");
  }

  try {
    auto object = toBEncodedObj(torrent);
    BEncoding::EncodeToFile(object, outputPath);
  } catch (const TorrentException &) {
    throw; // Re-throw our exceptions
  } catch (const std::exception &e) {
    throw TorrentException(ErrorCode::FileWriteError,
                          std::string("Failed to save torrent file: ") +
                              e.what());
  }
}

TorrentPtr Torrent::create(const fs::path& path, std::vector<std::string> trackers,
                  int pieceSize, std::string comment) {
  std::string name;
  std::vector<FileItem> files;

  // Check if path exists
  if (!std::filesystem::exists(path)) {
    throw TorrentException(ErrorCode::FileNotFound,
                           "Path does not exist: " + path.string());
  }

  if (std::filesystem::is_regular_file(path)) {
    // Single file mode
    std::error_code ec;
    size_t size = std::filesystem::file_size(path, ec);
    if (ec) {
      throw TorrentException(ErrorCode::FileReadError,
                             "Cannot get file size: " + path.string());
    }

    files.emplace_back(name, size, 0);
  } else if (std::filesystem::is_directory(path)) {
    // Directory mode
    name = path.filename().string();
    if (name.empty()) {
      name = path.parent_path().filename().string();
    }

     // Collect all files from directory
    files = Internal::collectFileWithinDir(path);

    if (files.empty()) {
      throw TorrentException(ErrorCode::InvalidParameter,
                             "Directory contains no files: " + path.string());
    }
  } else {
    throw TorrentException(ErrorCode::InvalidParameter,
                           "Path is neither a file nor a directory: " +
                               path.string());
  }

  // Use default trackers if none provided
  std::vector<std::string> trackersToUse = trackers;
  if (trackersToUse.empty()) {
    // Add a default tracker or leave empty based on your requirements
    // trackersToUse.push_back("http://tracker.example.com:8080/announce");
  }

  // Create torrent with empty location (will use the path provided)
  auto torrent =
      std::make_shared<Torrent>(name,
                                "", // Empty location - files have full paths
                                files, trackersToUse, pieceSize,
                                std::vector<Hash>(), // Hashes will be computed
                                16384,               // Default block size
                                false                // Not private
      );

  // Set metadata
  torrent->metadata_.comment = comment;
  torrent->metadata_.createdBy = "TestClient";
  torrent->metadata_.creationDate = std::time(nullptr);
  torrent->metadata_.encoding = "UTF-8";

  return torrent;
}

} // namespace LitTorrent
