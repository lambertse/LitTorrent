#pragma once

#include <ctime>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Component/Observable/Observable.h"

namespace LitTorrent {
// IPEndPoint structure to represent network endpoint
struct IPEndPoint {
  std::string address;
  int port;

  IPEndPoint(const std::string &addr, int p) : address(addr), port(p) {}
};

enum class TrackerEvent { STARTED, PAUSED, STOPPED };
class Torrent;
class Tracker {
public:
  explicit Tracker(const std::string &address);
  ~Tracker();

  std::string getAddress() const;
  void update(std::shared_ptr<Torrent> torrent, TrackerEvent event,
              std::string peerID, int port);
  void resetLastRequest();

private:
  void request(const std::string &url);
  bool handleResponse(const struct HTTPResponse& response);

private:
  std::string address_;
  time_t lastPeerRequest_;
  int peerRequestInterval_;
  Observable<std::vector<IPEndPoint>> peerListUpdated_; 
};

} // namespace LitTorrent
