#pragma once

#include <array>
#include <filesystem>
namespace LitTorrent {
using SHA1Hash = std::array<uint8_t, 20>;
using Buffer = std::string;
namespace fs {
using namespace std::filesystem;
}
} // namespace LitTorrent
