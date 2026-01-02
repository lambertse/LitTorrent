#pragma once

#include "LitTorrent/TorrentMetadata.h"
#include <functional>
#include <vector>

namespace LitTorrent {

// Callback when a piece is verified
using PieceVerifiedCallback = std::function<void(int pieceIndex, bool success)>;

class PieceVerifier {
public:
  PieceVerifier(const std::vector<Hash> &expectedHashes);

  // Verify a piece against its expected hash (returns true if valid)
  bool verify(int pieceIndex, const std::vector<uint8_t> &data);

  // Set callback for piece verification
  void setPieceVerifiedCallback(PieceVerifiedCallback callback);

  // Check if a piece is verified
  bool isPieceVerified(int pieceIndex) const;

  // Get verification status for all pieces
  const std::vector<bool> &getVerificationStatus() const;

  // Reset verification status
  void reset();

private:
  const std::vector<Hash> &expectedHashes_;
  std::vector<bool> verified_;
  PieceVerifiedCallback callback_;

  Hash computeHash(const std::vector<uint8_t> &data) const;
};

} // namespace LitTorrent
