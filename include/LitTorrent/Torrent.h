#pragma once

#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace LitTorrent {
// Forward declarations
class FileItem;
class Tracker;

class Torrent {
private:
  std::string name_;
  std::optional<bool> isPrivate_;
  std::vector<FileItem> files_;
  std::string downloadDirectory_;
  std::vector<std::unique_ptr<Tracker>> trackers_;
  std::string comment_;
  std::string createdBy_;
  std::time_t creationDate_;
  std::string encoding_;
  int blockSize_;
  int pieceSize_;
  std::vector<std::string> pieceHashes_;
  std::vector<bool> isPieceVerified_;
  std::vector<std::vector<bool>> isBlockAcquired_;
  std::string infoHash_;

public:
  // Constructor
  Torrent(std::string name, std::string location, std::vector<FileItem> files,
          std::vector<std::string> trackers, int pieceSize,
          std::vector<std::string> pieceHashes, int blockSize = 16384,
          bool isPrivate = false);

  // Destructor
  ~Torrent();

  // Getters
  std::string getName() const;
  std::optional<bool> getIsPrivate() const;
  const std::vector<FileItem> &getFiles() const;
  std::vector<FileItem> &getFiles();
  std::string getFileDirectory() const;
  std::string getDownloadDirectory() const;
  // const std::vector<Tracker> &getTrackers() const;
  // std::vector<Tracker> &getTrackers();
  std::string getComment() const;
  std::string getCreatedBy() const;
  std::time_t getCreationDate() const;
  std::string getEncoding() const;
  // int getBlockSize() const;
  // int getPieceSize() const;
  const std::vector<std::string> &getPieceHashes() const;
  int getPieceCount() const;
  const std::vector<bool> &getIsPieceVerified() const;
  const std::vector<bool> &getIsBlockAcquired() const;

  // Setters
  void setName(const std::string &name);
  void setIsPrivate(const std::optional<bool> &isPrivate);
  void setDownloadDirectory(const std::string &directory);
  void setComment(const std::string &comment);
  void setCreatedBy(const std::string &createdBy);
  void setCreationDate(std::time_t date);
  void setEncoding(const std::string &encoding);
  void setBlockSize(int size);
  void setPieceSize(int size);
  void setPieceHashes(const std::vector<std::string> &hashes);

private:
  size_t getTotalSize() const;

public:
  int getPieceSize(int pieceIdx) const;
  int getBlockSize(int pieceIdx, int blockIdx) const;
  int getBlockCount(int pieceIdx) const;

  std::string readPiece(int pieceIdx) const;
  std::string getHash(int pieceIdx) const;

  std::string read(int start, int count) const;
  void write(int start, std::string bytes) const;
};

} // namespace LitTorrent
