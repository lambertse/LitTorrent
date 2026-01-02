#include "FileItem.h"

namespace LitTorrent {
size_t FileItem::getSize() const { return size_; }
int FileItem::getOffset() const { return offset_; }
std::filesystem::path FileItem::getFilePath() const { return path_; }
} // namespace LitTorrent
