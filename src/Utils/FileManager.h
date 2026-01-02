#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace LitTorrent {

// Forward declaration
class FileItem;

// Manages file I/O operations with caching and error handling
// Throws TorrentException on errors
class FileManager {
public:
  explicit FileManager(const std::vector<FileItem> &files);
  ~FileManager();

  // Disable copy, allow move
  FileManager(const FileManager &) = delete;
  FileManager &operator=(const FileManager &) = delete;
  FileManager(FileManager &&) = default;
  FileManager &operator=(FileManager &&) = default;

  // Read data from the file set (throws on error)
  std::vector<uint8_t> read(size_t start, size_t count) const;

  // Write data to the file set (throws on error)
  void write(size_t start, const std::vector<uint8_t> &buffer);

  // Ensure all files exist and are properly sized (throws on error)
  void ensureFilesExist();

  // Close all open file handles
  void closeAll();

private:
  struct FileHandle {
    std::fstream stream;
    std::filesystem::path path;
    size_t size;
    size_t offset;
  };

  const std::vector<FileItem> &files_;
  mutable std::unordered_map<std::string, std::unique_ptr<FileHandle>> handles_;
  mutable std::mutex mutex_;

  FileHandle *getOrOpenFile(const std::filesystem::path &path,
                           size_t size, size_t offset, bool write) const;
};

} // namespace LitTorrent
