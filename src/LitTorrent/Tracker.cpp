#include "LitTorrent/Tracker.h"
#include "LitTorrent/BEncoding.h"
#include "LitTorrent/Torrent.h"
#include "LitTorrent/Tracker.h"
#include "../Utils/HTTPUtils.h"
#include "Logger.h"
#include <algorithm>
#include <cstddef>
#include <sstream>

namespace LitTorrent {

namespace {
namespace Internal {
static std::string trackerEventToString(const TrackerEvent &event) {
  switch (event) {
  case LitTorrent::TrackerEvent::STARTED:
    return "started";
  case LitTorrent::TrackerEvent::PAUSED:
    return "paused";
  case LitTorrent::TrackerEvent::STOPPED:
    return "stopped";
  default:
    return "";
  }
}
}
}

// Constructor
Tracker::Tracker(const std::string &address) : address_(address) {}

// Destructor
Tracker::~Tracker() {}

// Getter
std::string Tracker::getAddress() const { return address_; }

void Tracker::update(std::shared_ptr<Torrent> torrent, TrackerEvent event,
                     std::string peerID, int port) {
  if (event == TrackerEvent::STARTED &&
      time(0) < lastPeerRequest_ + peerRequestInterval_) {
    return;
  }

  auto hash = torrent->getInfoHash();
  lastPeerRequest_ = time(0);
  std::ostringstream oss;
  oss << address_ << "?info_hash=" << std::string(hash.begin(), hash.end())
      << "&peer_id=" << peerID << "&port=" << port
      << "&uploaded=" << torrent->getUploaded()
      << "&downloaded=" << torrent->getDownloaded()
      << "&left=" << torrent->getLeft()
      << "&event=" << Internal::trackerEventToString(event) << "&compact=1";

  LOG_INFO("Request to URL: %s", oss.str().c_str());
  request(oss.str());
}

void Tracker::resetLastRequest() { lastPeerRequest_ = time(0); }

void Tracker::request(const std::string &url) {
  // TBD
}

bool Tracker::handleResponse(const struct HTTPResponse& response){
    if(!response.success) {
        return false;
    } 
    
    BEncodedValuePtr res = BEncoding::Decode(response.body); 
    if(res == NULL) {return false;}
    BEncodedDict resDict= res->GetDictionary();

    peerRequestInterval_ = resDict["interval"]->GetNumber();
    ByteArray peerInfo = resDict["peers"]->GetByteArray();

    std::vector<IPEndPoint> endpoints;
    // Update peer list 
    for(int i = 0; i < peerInfo.size(); i++){
      int offset = i * 6;
      std::string addr = std::to_string(peerInfo[offset]) + "." +
                         std::to_string(peerInfo[offset + 1]) + "." +
                         std::to_string(peerInfo[offset + 2]) + "." +
                         std::to_string(peerInfo[offset + 3]);
      // Read the port (big-endian) at offset + 4
      uint16_t port = (static_cast<uint16_t>(peerInfo[offset + 4]) << 8) |
                      static_cast<uint16_t>(peerInfo[offset + 5]);
      endpoints.push_back(IPEndPoint(addr, port));
    }
    //

    peerListUpdated_.notify(endpoints);

    return true;
}
} // namespace LitTorrent
