#include "LitTorrent/Torrent.h"

#include "FileItem.h"
#include "LitTorrent/Tracker.h"
#include "../Utils/SHA1.h"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <locale>
#include <memory>

namespace LitTorrent {
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
    tracker->addPeerListUpdatedHandler();
    trackers_.push_back(tracker);
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

// Getters
std::string Torrent::getName() const { return name_; }

std::optional<bool> Torrent::getIsPrivate() const { return isPrivate_; }

const std::vector<FileItem> &Torrent::getFiles() const { return files_; }

std::vector<FileItem> &Torrent::getFiles() { return files_; }

std::string Torrent::getFileDirectory() const {
  if (files_.size() > 1) {
    return name_ + std::filesystem::path::preferred_separator;
  }
  return "";
}

std::string Torrent::getDownloadDirectory() const { return downloadDirectory_; }

std::string Torrent::getComment() const { return comment_; }

std::string Torrent::getCreatedBy() const { return createdBy_; }

std::time_t Torrent::getCreationDate() const { return creationDate_; }

std::string Torrent::getEncoding() const { return encoding_; }

// int Torrent::getBlockSize() const { return blockSize_; }
//
// int Torrent::getPieceSize() const { return pieceSize_; }

int Torrent::getPieceCount() const { return pieceHashes_.size(); }

int Torrent::getPieceSize(int pieceIdx) const {
  int pieceCount = getPieceCount();
  assert(pieceIdx >= pieceCount);
  if (pieceIdx != pieceCount - 1) {
    return pieceSize_;
  }
  return getTotalSize() % pieceSize_;
}

int Torrent::getBlockSize(int pieceIdx, int blockIdx) const {
  int blockCount = getBlockCount(pieceIdx);
  assert(blockIdx >= blockCount);
  if (blockIdx != blockCount - 1) {
    return blockSize_;
  }
  return getPieceSize(pieceIdx) % blockSize_;
}

int Torrent::getBlockCount(int pieceIdx) const {
  return ceil(getPieceSize(pieceIdx) * 1.0 / blockSize_);
}

std::string Torrent::readPiece(int pieceIdx) const {

}

std::string Torrent::getHash(int pieceIdx) const {
    std::string bytes = readPiece(pieceIdx);
    return SHA1::computeHash(bytes); 
}

  std::string Torrent::read(int start, int count) const{

}
  void Torrent::write(int start, std::string bytes) const{

}

const std::vector<std::string> &Torrent::getPieceHashes() const {
  return pieceHashes_;
}

const std::vector<bool> &Torrent::getIsPieceVerified() const {
  return isPieceVerified_;
}

// Setters
void Torrent::setName(const std::string &name) { this->name_ = name; }

void Torrent::setIsPrivate(const std::optional<bool> &isPrivate) {
  this->isPrivate_ = isPrivate;
}

void Torrent::setDownloadDirectory(const std::string &directory) {
  this->downloadDirectory_ = directory;
}

void Torrent::setComment(const std::string &comment) {
  this->comment_ = comment;
}

void Torrent::setCreatedBy(const std::string &createdBy) {
  this->createdBy_ = createdBy;
}

void Torrent::setCreationDate(std::time_t date) { this->creationDate_ = date; }

void Torrent::setEncoding(const std::string &encoding) {
  this->encoding_ = encoding;
}

void Torrent::setBlockSize(int size) { this->blockSize_ = size; }

void Torrent::setPieceSize(int size) { this->pieceSize_ = size; }

void Torrent::setPieceHashes(const std::vector<std::string> &hashes) {
  this->pieceHashes_ = hashes;
}

size_t Torrent::getTotalSize() const {
  int totalSize = 0;
  for (auto &file : files_) {
    totalSize += file.getSize();
  }
  return totalSize;
}

} // namespace LitTorrent
