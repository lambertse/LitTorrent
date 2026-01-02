#include "FileManager.h"
#include "Error.h"
#include "FileItem.h"
#include <algorithm>

namespace LitTorrent {

FileManager::FileManager(const std::vector<FileItem> &files) : files_(files) {}

FileManager::~FileManager() { closeAll(); }

FileManager::FileHandle *
FileManager::getOrOpenFile(const std::filesystem::path &path, size_t size,
                           size_t offset, bool write) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string pathStr = path.string();
  auto it = handles_.find(pathStr);

  if (it != handles_.end()) {
    return it->second.get();
  }

  // Create new handle
  auto handle = std::make_unique<FileHandle>();
  handle->path = path;
  handle->size = size;
  handle->offset = offset;

  // Create parent directories if they don't exist
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec) {
    throw TorrentException(ErrorCode::FileAccessDenied,
                           "Cannot create directory: " +
                               path.parent_path().string());
  }

  // Open file
  std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::in;
  if (write) {
    mode |= std::ios_base::out;

    // Create file if it doesn't exist
    if (!std::filesystem::exists(path)) {
      std::ofstream create(path, std::ios_base::binary);
      if (!create) {
        throw TorrentException(ErrorCode::FileAccessDenied,
                               "Cannot create file: " + path.string());
      }
      // Resize to expected size
      create.close();
      std::filesystem::resize_file(path, size, ec);
      if (ec) {
        throw TorrentException(ErrorCode::FileWriteError,
                               "Cannot resize file: " + path.string());
      }
    }
  }

  handle->stream.open(path, mode);
  if (!handle->stream.is_open()) {
    if (!std::filesystem::exists(path)) {
      throw TorrentException(ErrorCode::FileNotFound,
                             "File not found: " + path.string());
    }
    throw TorrentException(ErrorCode::FileAccessDenied,
                           "Cannot open file: " + path.string());
  }

  auto *ptr = handle.get();
  handles_[pathStr] = std::move(handle);
  return ptr;
}

std::vector<uint8_t> FileManager::read(size_t start, size_t count) const {
  std::vector<uint8_t> buffer(count, 0);
  size_t end = start + count;

  for (const auto &file : files_) {
    size_t fileStart = file.getOffset();
    size_t fileEnd = file.getOffset() + file.getSize();

    // Check if this file overlaps with the requested range
    if (fileStart < end && fileEnd > start) {
      // Calculate offset within the file to start reading
      size_t fstart = (start > fileStart) ? (start - fileStart) : 0;

      // Calculate how many bytes to read from this file
      size_t readEnd = std::min(end, fileEnd);
      size_t flength = readEnd - std::max(start, fileStart);

      // Calculate offset in the output buffer
      size_t bstart = (fileStart > start) ? (fileStart - start) : 0;

      auto *handle = getOrOpenFile(file.getFilePath(), file.getSize(),
                                   file.getOffset(), false);

      handle->stream.seekg(fstart);
      if (!handle->stream.read(reinterpret_cast<char *>(&buffer[bstart]),
                               flength)) {
        throw TorrentException(ErrorCode::FileReadError,
                               "Cannot read from: " +
                                   file.getFilePath().string());
      }
    }
  }

  return buffer;
}

void FileManager::write(size_t start, const std::vector<uint8_t> &buffer) {
  size_t end = start + buffer.size();

  for (const auto &file : files_) {
    size_t fileStart = file.getOffset();
    size_t fileEnd = file.getOffset() + file.getSize();

    // Check if this file overlaps with the requested range
    if (fileStart < end && fileEnd > start) {
      size_t fstart = (start > fileStart) ? (start - fileStart) : 0;
      size_t writeEnd = std::min(end, fileEnd);
      size_t writeLen = writeEnd - std::max(fileStart, start);

      // Calculate offset in the input buffer
      size_t bstart = (fileStart > start) ? (fileStart - start) : 0;

      auto *handle = getOrOpenFile(file.getFilePath(), file.getSize(),
                                   file.getOffset(), true);

      handle->stream.seekp(fstart);
      if (!handle->stream.write(reinterpret_cast<const char *>(&buffer[bstart]),
                                writeLen)) {
        throw TorrentException(ErrorCode::FileWriteError,
                               "Cannot write to: " +
                                   file.getFilePath().string());
      }
      handle->stream.flush();
    }
  }
}

void FileManager::ensureFilesExist() {
  std::error_code ec;

  for (const auto &file : files_) {
    std::filesystem::path path = file.getFilePath();

    // Create parent directories
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
      throw TorrentException(ErrorCode::FileAccessDenied,
                             "Cannot create directory: " +
                                 path.parent_path().string());
    }

    // Create file if it doesn't exist
    if (!std::filesystem::exists(path)) {
      std::ofstream create(path, std::ios_base::binary);
      if (!create) {
        throw TorrentException(ErrorCode::FileAccessDenied,
                               "Cannot create file: " + path.string());
      }
      create.close();

      // Resize to expected size
      std::filesystem::resize_file(path, file.getSize(), ec);
      if (ec) {
        throw TorrentException(ErrorCode::FileWriteError,
                               "Cannot resize file: " + path.string());
      }
    }
  }
}

void FileManager::closeAll() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[path, handle] : handles_) {
    if (handle->stream.is_open()) {
      handle->stream.close();
    }
  }
  handles_.clear();
}

} // namespace LitTorrent
