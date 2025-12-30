#pragma once

#include "LitTorrent/BEncoding.h"
namespace LitTorrent {

class ByteIterator {
private:
  const ByteArray &data;
  size_t position;

public:
  ByteIterator(const ByteArray &d) : data(d), position(0) {}

  uint8_t Current() const;
  bool MoveNext();
  bool HasMore() const;
};

class BEncodingImpl {
public:
  static BEncodedValuePtr Decode(const ByteArray &bytes);
  static BEncodedValuePtr DecodeFile(const std::string &path);

  static ByteArray Encode(BEncodedValuePtr obj); 
  static void EncodeToFile(BEncodedValuePtr obj, const std::string &path);

  static std::string GetFormattedString(BEncodedValuePtr obj, int depth = 0);

// Private methods:
private:
  // Decode methods
  static BEncodedValuePtr DecodeNextObject(ByteIterator &iterator);
  static BEncodedValuePtr DecodeDictionary(ByteIterator &iterator);
  static BEncodedValuePtr DecodeList(ByteIterator &iterator);
  static BEncodedValuePtr DecodeByteArray(ByteIterator &iterator);
  static BEncodedValuePtr DecodeNumber(ByteIterator &iterator);
  // Encode methods
  static void EncodeNextObject(std::vector<uint8_t> &buffer,
                               BEncodedValuePtr obj);
  static void EncodeByteArray(std::vector<uint8_t> &buffer,
                              const ByteArray &body);

  static void EncodeString(std::vector<uint8_t> &buffer,
                           const std::string &input);

  static void EncodeNumber(std::vector<uint8_t> &buffer, int64_t input);
  static void EncodeList(std::vector<uint8_t> &buffer,
                         const BEncodedList &input);
  static void EncodeDictionary(std::vector<uint8_t> &buffer,
                               const BEncodedDict &input);

  // Helper methods
  static std::string ByteArrayToHexString(const ByteArray &bytes);
  static std::string ByteArrayToString(const ByteArray &bytes);

private:
  static std::string GetFormattedStringByteArray(const ByteArray &obj);
  static std::string GetFormattedStringList(const BEncodedList &obj, int depth);
  static std::string GetFormattedStringDict(const BEncodedDict &obj, int depth);
};
} // namespace LitTorrent
