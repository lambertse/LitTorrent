#include "LitTorrent/Torrent.h"
#include "PieceVerifier.h"
#include "../Utils/FileManager.h"
#include "../Utils/SHA1.h"
#include "FileItem.h"
#include "Error.h"
#include "LitTorrent/BEncoding.h"
#include "LitTorrent/Tracker.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

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
                 int pieceSize, std::vector<Hash> pieceHashes, int blockSize,
                 bool isPrivate)
    : files_(std::move(files)), downloadDirectory_(std::move(location)),
      totalSize_(0), blockAcquired_() {

  metadata_.name = std::move(name);
  metadata_.isPrivate = isPrivate;
  metadata_.blockSize = blockSize;
  metadata_.pieceSize = pieceSize;
  metadata_.pieceHashes = std::move(pieceHashes);
  metadata_.creationDate = std::time(nullptr);

  // Calculate total size
  totalSize_ = calculateTotalSize();

  // Initialize trackers
  for (auto &trackerUrl : trackers) {
    auto tracker = std::make_unique<Tracker>(trackerUrl);
    trackers_.push_back(std::move(tracker));
  }

  int pieceCount = static_cast<int>(std::ceil(totalSize_ * 1.0 / pieceSize));

  // Initialize piece hashes if not provided
  if (metadata_.pieceHashes.empty()) {
    metadata_.pieceHashes.resize(pieceCount);
    for (int i = 0; i < pieceCount; i++) {
      auto pieceData = readPiece(i);
      std::string hashStr =
          SHA1::computeHash(std::string(pieceData.begin(), pieceData.end()));
      metadata_.pieceHashes[i] = BytesToHash(hashStr);
    }
  }

  // Initialize block tracking
  blockAcquired_.resize(pieceCount);
  for (int i = 0; i < pieceCount; i++) {
    blockAcquired_[i].resize(getBlockCount(i), false);
  }

  // Initialize file manager
  fileManager_ = std::make_unique<FileManager>(files_);

  // Initialize verifier
  verifier_ = std::make_unique<PieceVerifier>(metadata_.pieceHashes);

  // Compute info hash (placeholder - should be computed from bencoded info dict)
  metadata_.infoHash = Hash{};
}

// Destructor
Torrent::~Torrent() {
  if (fileManager_) {
    fileManager_->closeAll();
  }
}

size_t Torrent::calculateTotalSize() const {
  size_t total = 0;
  for (const auto &file : files_) {
    total += file.getSize();
  }
  return total;
}

void Torrent::validatePieceIndex(int pieceIdx) const {
  if (pieceIdx < 0 || pieceIdx >= getPieceCount()) {
    throw TorrentException(ErrorCode::InvalidPieceIndex,
                           "Piece index " + std::to_string(pieceIdx) +
                               " out of range [0, " +
                               std::to_string(getPieceCount()) + ")");
  }
}

void Torrent::validateBlockIndex(int pieceIdx, int blockIdx) const {
  validatePieceIndex(pieceIdx);
  if (blockIdx < 0 || blockIdx >= getBlockCount(pieceIdx)) {
    throw TorrentException(ErrorCode::InvalidBlockIndex,
                           "Block index " + std::to_string(blockIdx) +
                               " out of range [0, " +
                               std::to_string(getBlockCount(pieceIdx)) + ")");
  }
}

int Torrent::getPieceCount() const {
  return static_cast<int>(metadata_.pieceHashes.size());
}

int Torrent::getPieceSize(int pieceIdx) const {
  validatePieceIndex(pieceIdx);

  if (pieceIdx != getPieceCount() - 1) {
    return metadata_.pieceSize;
  }

  // Last piece might be smaller
  size_t remainder = totalSize_ % metadata_.pieceSize;
  return remainder == 0 ? metadata_.pieceSize : static_cast<int>(remainder);
}

int Torrent::getBlockSize(int pieceIdx, int blockIdx) const {
  validateBlockIndex(pieceIdx, blockIdx);

  int blockCount = getBlockCount(pieceIdx);
  if (blockIdx != blockCount - 1) {
    return metadata_.blockSize;
  }

  // Last block might be smaller
  int remainder = getPieceSize(pieceIdx) % metadata_.blockSize;
  return remainder == 0 ? metadata_.blockSize : remainder;
}

int Torrent::getBlockCount(int pieceIdx) const {
  validatePieceIndex(pieceIdx);
  return static_cast<int>(
      std::ceil(getPieceSize(pieceIdx) * 1.0 / metadata_.blockSize));
}

bool Torrent::isPieceVerified(int pieceIdx) const {
  validatePieceIndex(pieceIdx);
  return verifier_->isPieceVerified(pieceIdx);
}

int Torrent::calculateBlockOffset(int pieceIdx, int blockIdx) const {
  return metadata_.pieceSize * pieceIdx + blockIdx * metadata_.blockSize;
}

std::vector<uint8_t> Torrent::readPiece(int pieceIdx) const {
  validatePieceIndex(pieceIdx);
  return read(metadata_.pieceSize * pieceIdx, getPieceSize(pieceIdx));
}

std::vector<uint8_t> Torrent::readBlock(int pieceIdx, int blockIdx) const {
  validateBlockIndex(pieceIdx, blockIdx);
  int offset = calculateBlockOffset(pieceIdx, blockIdx);
  int length = getBlockSize(pieceIdx, blockIdx);
  return read(offset, length);
}

bool Torrent::writeBlock(int pieceIdx, int blockIdx,
                         const std::vector<uint8_t> &data) {
  validateBlockIndex(pieceIdx, blockIdx);

  int expectedSize = getBlockSize(pieceIdx, blockIdx);
  if (static_cast<int>(data.size()) != expectedSize) {
    throw TorrentException(ErrorCode::InvalidParameter,
                          "Block size mismatch: expected " +
                              std::to_string(expectedSize) + ", got " +
                              std::to_string(data.size()));
  }

  int offset = calculateBlockOffset(pieceIdx, blockIdx);
  std::vector<uint8_t> buffer(data.begin(), data.end());

  write(offset, buffer);

  // Mark block as acquired
  {
    std::lock_guard<std::mutex> lock(mutex_);
    blockAcquired_[pieceIdx][blockIdx] = true;
  }

  // Check if all blocks for this piece are acquired
  bool allBlocksAcquired = true;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (bool acquired : blockAcquired_[pieceIdx]) {
      if (!acquired) {
        allBlocksAcquired = false;
        break;
      }
    }
  }

  // If all blocks acquired, verify the piece
  if (allBlocksAcquired) {
    auto pieceData = readPiece(pieceIdx);
    bool verified = verifier_->verify(pieceIdx, pieceData);

    if (!verified) {
      // Hash mismatch - reset block tracking
      LOG_INFO("Hash verification failed for piece %d", pieceIdx);
      std::lock_guard<std::mutex> lock(mutex_);
      std::fill(blockAcquired_[pieceIdx].begin(),
                blockAcquired_[pieceIdx].end(), false);
      return false;
    }

    LOG_INFO("Piece %d verified successfully", pieceIdx);
    return true;
  }

  return true;
}

bool Torrent::writePiece(int pieceIdx, const std::vector<uint8_t> &data) {
  validatePieceIndex(pieceIdx);

  int expectedSize = getPieceSize(pieceIdx);
  if (static_cast<int>(data.size()) != expectedSize) {
    throw TorrentException(ErrorCode::InvalidParameter,
                          "Piece size mismatch: expected " +
                              std::to_string(expectedSize) + ", got " +
                              std::to_string(data.size()));
  }

  write(metadata_.pieceSize * pieceIdx, data);

  // Verify the piece
  bool verified = verifier_->verify(pieceIdx, data);

  if (verified) {
    // Mark all blocks as acquired
    std::lock_guard<std::mutex> lock(mutex_);
    std::fill(blockAcquired_[pieceIdx].begin(), blockAcquired_[pieceIdx].end(),
              true);
  }

  return verified;
}

const Hash &Torrent::getHash(int pieceIdx) const {
  validatePieceIndex(pieceIdx);
  return metadata_.pieceHashes[pieceIdx];
}

const Hash &Torrent::getInfoHash() const { return metadata_.infoHash; }

std::vector<uint8_t> Torrent::read(size_t start, size_t count) const {
  if (!fileManager_) {
    throw TorrentException(ErrorCode::FileReadError, "FileManager not initialized");
  }
  return fileManager_->read(start, count);
}

void Torrent::write(size_t start, const std::vector<uint8_t> &buffer) {
  if (!fileManager_) {
    throw TorrentException(ErrorCode::FileWriteError,
                           "FileManager not initialized");
  }

  std::vector<uint8_t> vec(buffer.begin(), buffer.end());
  fileManager_->write(start, vec);
}

void Torrent::setPieceVerifiedCallback(PieceVerifiedCallback callback) {
  verifier_->setPieceVerifiedCallback(std::move(callback));
}

void Torrent::ensureFilesExist() {
  if (!fileManager_) {
    throw TorrentException(ErrorCode::FileAccessDenied,
                          "FileManager not initialized");
  }
  fileManager_->ensureFilesExist();
}

void Torrent::closeFiles() {
  if (fileManager_) {
    fileManager_->closeAll();
  }
}

double Torrent::getProgress() const {
  if (getPieceCount() == 0) {
    return 0.0;
  }

  int verifiedCount = 0;
  for (int i = 0; i < getPieceCount(); i++) {
    if (verifier_->isPieceVerified(i)) {
      verifiedCount++;
    }
  }

  return static_cast<double>(verifiedCount) / getPieceCount() * 100.0;
}

size_t Torrent::getDownloadedBytes() const {
  size_t downloaded = 0;
  for (int i = 0; i < getPieceCount(); i++) {
    if (verifier_->isPieceVerified(i)) {
      downloaded += getPieceSize(i);
    }
  }
  return downloaded;
}

} // namespace LitTorrent
