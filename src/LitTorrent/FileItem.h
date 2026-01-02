#pragma once

#include <cstddef>
#include <filesystem>
namespace LitTorrent {
class FileItem {
public:
  FileItem(const std::filesystem::path &path, const size_t &size,
           const int &offset = 0)
      : offset_(offset), path_(path), size_(size) {}

  size_t getSize() const;
  int getOffset() const;
  std::filesystem::path getFilePath() const;

private:
  std::filesystem::path path_;
  size_t size_;
  int offset_;
};
} // namespace LitTorrent
