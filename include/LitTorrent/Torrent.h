#pragma once

#include "LitTorrent/BEncoding.h"
#include "PieceVerifier.h"
#include "TorrentMetadata.h"
#include "Define.h"
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace LitTorrent {

// Forward declarations
class FileItem;
class Tracker;
using TorrentPtr = std::shared_ptr<class Torrent>;

class Torrent : public std::enable_shared_from_this<Torrent> {
public:
  // Constructor
  Torrent(std::string name, std::string location, std::vector<FileItem> files,
          std::vector<std::string> trackers, int pieceSize,
          std::vector<Hash> pieceHashes, int blockSize = 16384,
          bool isPrivate = false);

  // Destructor
  ~Torrent();

  // Disable copy, allow move
  Torrent(const Torrent &) = delete;
  Torrent &operator=(const Torrent &) = delete;
  Torrent(Torrent &&) = delete;
  Torrent &operator=(Torrent &&) = delete;

  // Piece operations (throw on error)
  int getPieceCount() const;
  int getPieceSize(int pieceIdx) const;
  bool isPieceVerified(int pieceIdx) const;
  std::vector<uint8_t> readPiece(int pieceIdx) const;
  bool writePiece(int pieceIdx, const std::vector<uint8_t>& data);

  // Block operations (throw on error)
  int getBlockCount(int pieceIdx) const;
  int getBlockSize(int pieceIdx, int blockIdx) const;
  std::vector<uint8_t> readBlock(int pieceIdx, int blockIdx) const;
  bool writeBlock(int pieceIdx, int blockIdx, const std::vector<uint8_t>& data);

  // Hash operations
  const Hash &getHash(int pieceIdx) const;
  const Hash &getInfoHash() const;

  int getUploaded() const;
  int getDownloaded() const;
  int getVerifiedPieceCount() const;
  int getLeft() const;

  // Callback management
  void setPieceVerifiedCallback(PieceVerifiedCallback callback);

  // File operations (throw on error)
  void ensureFilesExist();
  void closeFiles();

  // Metadata access
  const std::string &getName() const { return metadata_.name; }
  const std::vector<FileItem> &getFiles() const { return files_; }
  const std::string &getDownloadDirectory() const { return downloadDirectory_; }
  const TorrentMetadata &getMetadata() const { return metadata_; }
  size_t getTotalSize() const { return totalSize_; }

  // Progress tracking
  double getProgress() const;
  size_t getDownloadedBytes() const;

  // Static methods for serialization (throw on error)
  static TorrentPtr loadFromFile(fs::path filePath,
                                 fs::path downloadDir);
  static void saveToFile(TorrentPtr torrent, fs::path outputPath);

  static TorrentPtr fromBEncodedObj(BEncodedValuePtr object,
                                    const std::string &downloadPath);
  static BEncodedValuePtr toBEncodedObj(TorrentPtr torrent);

  static TorrentPtr create(const fs::path &path,
                           std::vector<std::string> trackers,
                           int pieceSize = 32768, std::string comment = "");

private:
  // Helper methods
  void validatePieceIndex(int pieceIdx) const;
  void validateBlockIndex(int pieceIdx, int blockIdx) const;
  size_t calculateTotalSize() const;
  int calculateBlockOffset(int pieceIdx, int blockIdx) const;

  std::vector<uint8_t> read(size_t start, size_t count) const;
  void write(size_t start, const std::vector<uint8_t>& buffer);

  static BEncodedValuePtr torrentInfoToBEncodedObj(TorrentPtr torrent);

  // Member variables
  TorrentMetadata metadata_;
  std::vector<FileItem> files_;
  std::string downloadDirectory_;
  std::vector<std::unique_ptr<Tracker>> trackers_;
  size_t totalSize_;
  int uploaded_;
  std::vector<std::vector<bool>> blockAcquired_;

  // File management
  std::unique_ptr<class FileManager> fileManager_;

  // Piece verification
  std::unique_ptr<PieceVerifier> verifier_;

  // Thread safety
  mutable std::mutex mutex_;
};

} // namespace LitTorrent
