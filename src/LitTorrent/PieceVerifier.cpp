#include "PieceVerifier.h"
#include "../Utils/SHA1.h"
#include "Error.h"
#include <algorithm>

namespace LitTorrent {

PieceVerifier::PieceVerifier(const std::vector<Hash> &expectedHashes)
    : expectedHashes_(expectedHashes), verified_(expectedHashes.size(), false) {
}

Hash PieceVerifier::computeHash(const std::vector<uint8_t> &data) const {
  std::string dataStr(data.begin(), data.end());
  std::string hashStr = SHA1::computeHash(dataStr);
  return BytesToHash(hashStr);
}

bool PieceVerifier::verify(int pieceIndex, const std::vector<uint8_t> &data) {
  if (pieceIndex < 0 ||
      pieceIndex >= static_cast<int>(expectedHashes_.size())) {
    throw TorrentException(ErrorCode::InvalidPieceIndex,
                           "Piece index " + std::to_string(pieceIndex) +
                               " out of range");
  }

  Hash computed = computeHash(data);
  bool matches = (computed == expectedHashes_[pieceIndex]);

  verified_[pieceIndex] = matches;

  if (callback_) {
    callback_(pieceIndex, matches);
  }

  return matches;
}

void PieceVerifier::setPieceVerifiedCallback(PieceVerifiedCallback callback) {
  callback_ = std::move(callback);
}

bool PieceVerifier::isPieceVerified(int pieceIndex) const {
  if (pieceIndex < 0 || pieceIndex >= static_cast<int>(verified_.size())) {
    return false;
  }
  return verified_[pieceIndex];
}

const std::vector<bool> &PieceVerifier::getVerificationStatus() const {
  return verified_;
}

void PieceVerifier::reset() {
  std::fill(verified_.begin(), verified_.end(), false);
}

} // namespace LitTorrent
