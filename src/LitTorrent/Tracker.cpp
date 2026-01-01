#include "LitTorrent/Tracker.h"
#include <algorithm>

namespace LitTorrent {
// Constructor
Tracker::Tracker(const std::string &address) : address(address) {}

// Destructor
Tracker::~Tracker() {}

// Getter
std::string Tracker::getAddress() const { return address; }

// Setter (private)
void Tracker::setAddress(const std::string &address) {
  this->address = address;
}

// Add event handler
void Tracker::addPeerListUpdatedHandler(const PeerListUpdatedHandler &handler) {
  peerListUpdatedHandlers.push_back(handler);
}

// Remove event handler
void Tracker::removePeerListUpdatedHandler(
    const PeerListUpdatedHandler &handler) {
  // Note: This is a simple implementation. For more robust comparison,
  // you might need to store handlers with identifiers or use a different
  // approach Since std::function doesn't have a comparison operator, this is a
  // simplified version In practice, you might want to use a different event
  // system

  // This is a placeholder - std::function cannot be compared directly
  // You would need to implement a proper event system with handler IDs
  // or use a library like Boost.Signals2
}

// Trigger the event
void Tracker::onPeerListUpdated(const std::vector<IPEndPoint> &peerList) {
  for (const auto &handler : peerListUpdatedHandlers) {
    if (handler) {
      handler(peerList);
    }
  }
}

} // namespace LitTorrent
