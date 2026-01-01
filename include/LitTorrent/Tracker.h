#pragma once

#include <functional>
#include <string>
#include <vector>

namespace LitTorrent {
// IPEndPoint structure to represent network endpoint
struct IPEndPoint {
  std::string address;
  int port;

  IPEndPoint(const std::string &addr, int p) : address(addr), port(p) {}
};

class Tracker {
public:
  // Event handler type for PeerListUpdated event
  using PeerListUpdatedHandler =
      std::function<void(const std::vector<IPEndPoint> &)>;

private:
  std::string address;
  std::vector<PeerListUpdatedHandler> peerListUpdatedHandlers;

public:
  explicit Tracker(const std::string &address);
  ~Tracker();

  // Getter
  std::string getAddress() const;

  // Event subscription methods
  void addPeerListUpdatedHandler(const PeerListUpdatedHandler &handler);
  void removePeerListUpdatedHandler(const PeerListUpdatedHandler &handler);

  // Method to trigger the event
  void onPeerListUpdated(const std::vector<IPEndPoint> &peerList);

private:
  // Setter (private to match C# behavior)
  void setAddress(const std::string &address);
};

} // namespace LitTorrent

