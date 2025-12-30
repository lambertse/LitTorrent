#include "BEncodingImpl.h"
#include "LitTorrent/BEncoding.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace LitTorrent {

const uint8_t DictionaryStart = 'd';  // 100
const uint8_t DictionaryEnd = 'e';    // 101
const uint8_t ListStart = 'l';        // 108
const uint8_t ListEnd = 'e';          // 101
const uint8_t NumberStart = 'i';      // 105
const uint8_t NumberEnd = 'e';        // 101
const uint8_t ByteArrayDivider = ':'; // 58

uint8_t ByteIterator::Current() const {
  if (position >= data.size())
    throw std::runtime_error("Iterator out of bounds");
  return data[position];
}

bool ByteIterator::MoveNext() {
  if (position < data.size() - 1) {
    position++;
    return true;
  }
  return false;
}

bool ByteIterator::HasMore() const { return position < data.size(); }

// Decode methods
BEncodedValuePtr BEncodingImpl::DecodeNextObject(ByteIterator &iterator) {
  uint8_t current = iterator.Current();

  if (current == DictionaryStart)
    return DecodeDictionary(iterator);

  if (current == ListStart)
    return DecodeList(iterator);

  if (current == NumberStart)
    return DecodeNumber(iterator);
  
  return DecodeByteArray(iterator);
}

BEncodedValuePtr BEncodingImpl::DecodeDictionary(ByteIterator &iterator) {
  BEncodedDict dict;
  std::vector<std::string> keys;

  while (iterator.MoveNext()) {
    if (iterator.Current() == DictionaryEnd)
      break;

    // All keys are valid UTF8 strings
    auto keyBytes = DecodeByteArray(iterator)->GetByteArray();
    std::string key(keyBytes.begin(), keyBytes.end());

    iterator.MoveNext();
    auto val = DecodeNextObject(iterator);

    keys.push_back(key);
    dict[key] = val;
  }

  // Verify incoming dictionary is sorted correctly
  auto sortedKeys = keys;
  std::sort(
      sortedKeys.begin(), sortedKeys.end(),
      [](const std::string &a, const std::string &b) {
        return ByteArrayToHexString(std::vector<uint8_t>(a.begin(), a.end())) <
               ByteArrayToHexString(std::vector<uint8_t>(b.begin(), b.end()));
      });

  if (keys != sortedKeys)
    throw std::runtime_error("error loading dictionary: keys not sorted");

  return BEncodedValue::CreateDictionary(dict);
}

BEncodedValuePtr BEncodingImpl::DecodeList(ByteIterator &iterator) {
  BEncodedList list;

  while (iterator.MoveNext()) {
    if (iterator.Current() == ListEnd)
      break;

    list.push_back(DecodeNextObject(iterator));
  }

  return BEncodedValue::CreateList(list);
}

BEncodedValuePtr BEncodingImpl::DecodeByteArray(ByteIterator &iterator) {
  std::vector<uint8_t> lengthBytes;

  // Scan until we get to divider
  do {
    if (iterator.Current() == ByteArrayDivider)
      break;

    lengthBytes.push_back(iterator.Current());
  } while (iterator.MoveNext());

  std::string lengthString(lengthBytes.begin(), lengthBytes.end());
  int length = std::stoi(lengthString);

  // Now read in the actual byte array
  ByteArray bytes;
  bytes.reserve(length);

  for (int i = 0; i < length; i++) {
    iterator.MoveNext();
    bytes.push_back(iterator.Current());
  }

  return BEncodedValue::CreateByteArray(bytes);
}

BEncodedValuePtr BEncodingImpl::DecodeNumber(ByteIterator &iterator) {
  std::vector<uint8_t> bytes;

  // Keep pulling bytes until we hit the end flag
  while (iterator.MoveNext()) {
    if (iterator.Current() == NumberEnd)
      break;

    bytes.push_back(iterator.Current());
  }

  std::string numAsString(bytes.begin(), bytes.end());
  int64_t number = std::stoll(numAsString);

  return BEncodedValue::CreateNumber(number);
}

// Encode methods
void BEncodingImpl::EncodeNextObject(std::vector<uint8_t> &buffer,
                                     BEncodedValuePtr obj) {
  switch (obj->GetType()) {
  case BEncodedValue::Type::ByteArray:
    EncodeByteArray(buffer, obj->GetByteArray());
    break;
  case BEncodedValue::Type::Number:
    EncodeNumber(buffer, obj->GetNumber());
    break;
  case BEncodedValue::Type::List:
    EncodeList(buffer, obj->GetList());
    break;
  case BEncodedValue::Type::Dictionary:
    EncodeDictionary(buffer, obj->GetDictionary());
    break;
  }
}

void BEncodingImpl::EncodeByteArray(std::vector<uint8_t> &buffer,
                                    const ByteArray &body) {
  std::string lengthStr = std::to_string(body.size());
  buffer.insert(buffer.end(), lengthStr.begin(), lengthStr.end());
  buffer.push_back(ByteArrayDivider);
  buffer.insert(buffer.end(), body.begin(), body.end());
}

void BEncodingImpl::EncodeString(std::vector<uint8_t> &buffer,
                                 const std::string &input) {
  ByteArray bytes(input.begin(), input.end());
  EncodeByteArray(buffer, bytes);
}

void BEncodingImpl::EncodeNumber(std::vector<uint8_t> &buffer, int64_t input) {
  buffer.push_back(NumberStart);
  std::string numStr = std::to_string(input);
  buffer.insert(buffer.end(), numStr.begin(), numStr.end());
  buffer.push_back(NumberEnd);
}

void BEncodingImpl::EncodeList(std::vector<uint8_t> &buffer,
                               const BEncodedList &input) {
  buffer.push_back(ListStart);
  for (const auto &item : input)
    EncodeNextObject(buffer, item);
  buffer.push_back(ListEnd);
}

void BEncodingImpl::EncodeDictionary(std::vector<uint8_t> &buffer,
                                     const BEncodedDict &input) {
  buffer.push_back(DictionaryStart);

  // Sort keys by their raw bytes
  std::vector<std::string> sortedKeys;
  for (const auto &pair : input)
    sortedKeys.push_back(pair.first);

  std::sort(
      sortedKeys.begin(), sortedKeys.end(),
      [](const std::string &a, const std::string &b) {
        return ByteArrayToHexString(std::vector<uint8_t>(a.begin(), a.end())) <
               ByteArrayToHexString(std::vector<uint8_t>(b.begin(), b.end()));
      });

  for (const auto &key : sortedKeys) {
    EncodeString(buffer, key);
    EncodeNextObject(buffer, input.at(key));
  }
  buffer.push_back(DictionaryEnd);
}

// Helper methods
std::string BEncodingImpl::ByteArrayToHexString(const ByteArray &bytes) {
  std::stringstream ss;
  for (uint8_t byte : bytes) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte);
  }
  return ss.str();
}

std::string BEncodingImpl::ByteArrayToString(const ByteArray &bytes) {
  return std::string(bytes.begin(), bytes.end());
}

BEncodedValuePtr BEncodingImpl::Decode(const ByteArray &bytes) {
  ByteIterator iterator(bytes);
  return DecodeNextObject(iterator);
}

BEncodedValuePtr BEncodingImpl::DecodeFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Unable to open file");

  ByteArray bytes((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());

  return Decode(bytes);
}

ByteArray BEncodingImpl::Encode(BEncodedValuePtr obj) {
  ByteArray buffer;
  EncodeNextObject(buffer, obj);
  return buffer;
}

void BEncodingImpl::EncodeToFile(BEncodedValuePtr obj,
                                 const std::string &path) {
  ByteArray encoded = Encode(obj);
  std::ofstream file(path, std::ios::binary);
  if (!file)
    throw std::runtime_error("Unable to write file");

  file.write(reinterpret_cast<const char *>(encoded.data()), encoded.size());
}

std::string BEncodingImpl::GetFormattedString(BEncodedValuePtr obj, int depth) {
  switch (obj->GetType()) {
  case BEncodedValue::Type::ByteArray:
    return GetFormattedStringByteArray(obj->GetByteArray());
  case BEncodedValue::Type::Number:
    return std::to_string(obj->GetNumber());
  case BEncodedValue::Type::List:
    return GetFormattedStringList(obj->GetList(), depth);
  case BEncodedValue::Type::Dictionary:
    return GetFormattedStringDict(obj->GetDictionary(), depth);
  }
  return "";
}

std::string BEncodingImpl::GetFormattedStringByteArray(const ByteArray &obj) {
  std::stringstream ss;
  for (uint8_t byte : obj)
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(byte);

  ss << " (" << ByteArrayToString(obj) << ")";
  return ss.str();
}

std::string BEncodingImpl::GetFormattedStringList(const BEncodedList &obj,
                                                  int depth) {
  std::string pad1(depth * 2, ' ');
  std::string pad2((depth + 1) * 2, ' ');

  if (obj.empty())
    return "[]";

  if (obj[0]->GetType() == BEncodedValue::Type::Dictionary) {
    std::stringstream ss;
    ss << "\n" << pad1 << "[";
    for (const auto &item : obj)
      ss << pad2 << GetFormattedString(item, depth + 1);
    ss << "\n" << pad1 << "]";
    return ss.str();
  }

  std::stringstream ss;
  ss << "[ ";
  for (size_t i = 0; i < obj.size(); i++) {
    if (i > 0)
      ss << ", ";
    ss << GetFormattedString(obj[i]);
  }
  ss << " ]";
  return ss.str();
}

std::string BEncodingImpl::GetFormattedStringDict(const BEncodedDict &obj,
                                                  int depth) {
  std::string pad1(depth * 2, ' ');
  std::string pad2((depth + 1) * 2, ' ');

  std::stringstream ss;
  if (depth > 0)
    ss << "\n";
  ss << pad1 << "{";

  for (const auto &pair : obj) {
    ss << "\n" << pad2;
    std::string keyPadded = pair.first + ":";
    keyPadded.resize(15, ' ');
    ss << keyPadded << GetFormattedString(pair.second, depth + 1);
  }

  ss << "\n" << pad1 << "}";
  return ss.str();
}

} // namespace LitTorrent
