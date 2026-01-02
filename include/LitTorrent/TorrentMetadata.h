#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace LitTorrent {

// Represents a SHA-1 hash (20 bytes)
using Hash = std::array<uint8_t, 20>;

// Structure for immutable torrent metadata
struct TorrentMetadata {
  std::string name;
  std::optional<bool> isPrivate;
  std::string comment;
  std::string createdBy;
  std::time_t creationDate;
  std::string encoding;
  int blockSize;
  int pieceSize;
  std::vector<Hash> pieceHashes;
  Hash infoHash;

  TorrentMetadata()
      : creationDate(0), blockSize(16384), pieceSize(0), infoHash{} {}
};

// Convert Hash to hex string for display
inline std::string HashToHex(const Hash &hash) {
  static const char hex[] = "0123456789abcdef";
  std::string result;
  result.reserve(40);
  for (uint8_t byte : hash) {
    result.push_back(hex[byte >> 4]);
    result.push_back(hex[byte & 0x0F]);
  }
  return result;
}

// Convert raw bytes to Hash
inline Hash BytesToHash(const std::string &bytes) {
  if (bytes.size() != 20) {
    throw std::invalid_argument("Hash must be exactly 20 bytes");
  }
  Hash hash;
  std::copy(bytes.begin(), bytes.end(), hash.begin());
  return hash;
}

// Convert Hash to raw bytes
inline std::string HashToBytes(const Hash &hash) {
  return std::string(hash.begin(), hash.end());
}

} // namespace LitTorrent
