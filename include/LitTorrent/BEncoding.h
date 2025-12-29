#pragma once

#include <map>
#include <memory>
#include <string>

namespace LitTorrent {

class BEncodedValue;
class ByteIterator;

using ByteArray = std::vector<uint8_t>;
using BEncodedDict = std::map<std::string, std::shared_ptr<BEncodedValue>>;
using BEncodedList = std::vector<std::shared_ptr<BEncodedValue>>;
using BEncodedValuePtr = std::shared_ptr<BEncodedValue>;

class BEncodedValue {
public:
  enum class Type { ByteArray, Number, List, Dictionary };
  BEncodedValue(Type t) : type_(t), number_(0) {}

  static BEncodedValuePtr CreateByteArray(const ByteArray &data);
  static BEncodedValuePtr CreateNumber(int64_t num);
  static BEncodedValuePtr CreateList(const BEncodedList &lst);
  static BEncodedValuePtr CreateDictionary(const BEncodedDict &dict);

  Type GetType();

  ByteArray GetByteArray();
  int64_t GetNumber();
  BEncodedList GetList();
  BEncodedDict GetDictionary();

private:
  Type type_;
  ByteArray byteArray_;
  int64_t number_;
  BEncodedList list_;
  BEncodedDict dictionary_;
};

class BEncodingImpl;
class BEncoding {
public:
  BEncoding() = delete; // Prevent instantiation since all methods are static
  ~BEncoding() = delete;

  static BEncodedValuePtr Decode(const ByteArray &bytes);
  static BEncodedValuePtr DecodeFile(const std::string &path);

  static ByteArray Encode(BEncodedValuePtr obj);
  static void EncodeToFile(BEncodedValuePtr obj, const std::string &path);
};

} // namespace LitTorrent
