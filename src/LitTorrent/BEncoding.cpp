#include "LitTorrent/BEncoding.h"
#include "BEncodingImpl.h"
#include <cassert>
#include <cstddef>
#include <memory>

namespace LitTorrent {

BEncodedValuePtr BEncodedValue::CreateByteArray(const ByteArray &data) {
  auto val = std::make_shared<BEncodedValue>(Type::ByteArray);
  val->byteArray_ = data;
  return val;
}

BEncodedValuePtr BEncodedValue::CreateNumber(int64_t num) {
  auto val = std::make_shared<BEncodedValue>(Type::Number);
  val->number_ = num;
  return val;
}

BEncodedValuePtr BEncodedValue::CreateList(const BEncodedList &lst) {
  auto val = std::make_shared<BEncodedValue>(Type::List);
  val->list_ = lst;
  return val;
}

BEncodedValuePtr BEncodedValue::CreateDictionary(const BEncodedDict &dict) {
  auto val = std::make_shared<BEncodedValue>(Type::Dictionary);
  val->dictionary_ = dict;
  return val;
}

BEncodedValue::Type BEncodedValue::GetType() { return type_; }

ByteArray BEncodedValue::GetByteArray() { return byteArray_; }
int64_t BEncodedValue::GetNumber() { return number_; }
BEncodedList BEncodedValue::GetList() { return list_; }
BEncodedDict BEncodedValue::GetDictionary() { return dictionary_; }

BEncodedValuePtr BEncoding::Decode(const ByteArray &bytes) {
  return BEncodingImpl::Decode(bytes);
}
BEncodedValuePtr BEncoding::DecodeFile(const std::string &path) {
  return BEncodingImpl::DecodeFile(path);
}

ByteArray Encode(BEncodedValuePtr obj) { return BEncodingImpl::Encode(obj); }

void EncodeToFile(BEncodedValuePtr obj, const std::string &path) {
  return BEncodingImpl::EncodeToFile(obj, path);
}

} // namespace LitTorrent
