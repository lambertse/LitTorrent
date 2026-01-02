#pragma once

#include <stdexcept>
#include <string>
#include <system_error>

namespace LitTorrent {

// Error categories for different types of failures
enum class ErrorCode {
  Success = 0,
  FileNotFound,
  FileAccessDenied,
  FileReadError,
  FileWriteError,
  InvalidPieceIndex,
  InvalidBlockIndex,
  HashMismatch,
  InvalidTorrentFile,
  MissingInfoSection,
  MissingTrackers,
  NetworkError,
  InvalidParameter,
  OutOfBounds
};

class ErrorCategory : public std::error_category {
public:
  const char *name() const noexcept override { return "LitTorrent"; }

  std::string message(int ev) const override {
    switch (static_cast<ErrorCode>(ev)) {
    case ErrorCode::Success:
      return "Success";
    case ErrorCode::FileNotFound:
      return "File not found";
    case ErrorCode::FileAccessDenied:
      return "File access denied";
    case ErrorCode::FileReadError:
      return "Error reading file";
    case ErrorCode::FileWriteError:
      return "Error writing file";
    case ErrorCode::InvalidPieceIndex:
      return "Invalid piece index";
    case ErrorCode::InvalidBlockIndex:
      return "Invalid block index";
    case ErrorCode::HashMismatch:
      return "Hash verification failed";
    case ErrorCode::InvalidTorrentFile:
      return "Invalid torrent file format";
    case ErrorCode::MissingInfoSection:
      return "Missing info section in torrent";
    case ErrorCode::MissingTrackers:
      return "No trackers specified";
    case ErrorCode::NetworkError:
      return "Network error";
    case ErrorCode::InvalidParameter:
      return "Invalid parameter";
    case ErrorCode::OutOfBounds:
      return "Index out of bounds";
    default:
      return "Unknown error";
    }
  }
};

inline const ErrorCategory &GetErrorCategory() {
  static ErrorCategory category;
  return category;
}

inline std::error_code make_error_code(ErrorCode e) {
  return {static_cast<int>(e), GetErrorCategory()};
}

// Custom exception class
class TorrentException : public std::runtime_error {
public:
  explicit TorrentException(ErrorCode code)
      : std::runtime_error(GetErrorCategory().message(static_cast<int>(code))),
        code_(code) {}

  explicit TorrentException(ErrorCode code, const std::string &detail)
      : std::runtime_error(GetErrorCategory().message(static_cast<int>(code)) +
                           ": " + detail),
        code_(code) {}

  ErrorCode code() const { return code_; }

private:
  ErrorCode code_;
};

} // namespace LitTorrent

namespace std {
template <> struct is_error_code_enum<LitTorrent::ErrorCode> : true_type {};
} // namespace std
