#pragma once

#include "LitTorrent/BEncoding.h"
#include <ctime>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace LitTorrent {
// Forward declarations
class FileItem;
class Tracker;
using TorrentPtr = std::shared_ptr<class Torrent>;
using PieceVerifiedCallback = std::function<int(int)>;

class Torrent {
public:
  // Constructor
  Torrent(std::string name, std::string location, std::vector<FileItem> files,
          std::vector<std::string> trackers, int pieceSize,
          std::vector<std::string> pieceHashes, int blockSize = 16384,
          bool isPrivate = false);

  // Destructor
  ~Torrent();

private:
  size_t getTotalSize() const;
  void verify(int pieceIdx);

public:
    int getPieceCount() const;
    void setPieceVerifiedCallback(PieceVerifiedCallback callback);
    int getPieceSize(int pieceIdx) const;
    int getBlockSize(int pieceIdx, int blockIdx) const;
    int getBlockCount(int pieceIdx) const;

    std::string readPiece(int pieceIdx) const;
    std::string readBlock(int pieceIdx, int offset, int length);
    void writeBlock(int pieceIdx, int block, std::string bytes);

    std::string getHash(int pieceIdx) const;

    std::string read(int start, int count) const;
    void write(int start, const std::string &buffer) const;

  public:
    // Static method:
    static TorrentPtr BEncodedObjToTorrent(BEncodedValuePtr object);
    static BEncodedValuePtr torrentInfoToBEncodedObj(TorrentPtr torrent);
    static BEncodedValuePtr TorrentToBEncodedObj(TorrentPtr torrent);

    static TorrentPtr loadFromFile(std::filesystem::path filePath,
                                   std::filesystem::path downloadDir);
    static void saveToFile(TorrentPtr torrent);

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

    PieceVerifiedCallback pieceVerifiedCb_;
};

} // namespace LitTorrent
