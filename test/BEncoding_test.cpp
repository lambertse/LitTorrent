#include <gtest/gtest.h>
#include "LitTorrent/BEncoding.h"

using namespace LitTorrent;

class BEncodingDecodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }

    void TearDown() override {
        // Cleanup
    }
    
    // Helper to convert string to ByteArray
    ByteArray StringToByteArray(const std::string& str) {
        return ByteArray(str.begin(), str.end());
    }
};

// Number Decoding Tests
TEST_F(BEncodingDecodeTest, DecodePositiveNumber) {
    // "i42e" - integer 42
    ByteArray data = {'i', '4', '2', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(result->GetNumber(), 42);
}

TEST_F(BEncodingDecodeTest, DecodeNegativeNumber) {
    // "i-42e" - integer -42
    ByteArray data = {'i', '-', '4', '2', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(result->GetNumber(), -42);
}

TEST_F(BEncodingDecodeTest, DecodeZero) {
    // "i0e" - integer 0
    ByteArray data = {'i', '0', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(result->GetNumber(), 0);
}

TEST_F(BEncodingDecodeTest, DecodeLargeNumber) {
    // "i9223372036854775807e" - max int64_t
    std::string numStr = "i9223372036854775807e";
    ByteArray data = StringToByteArray(numStr);
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(result->GetNumber(), 9223372036854775807LL);
}

// ByteArray Decoding Tests
TEST_F(BEncodingDecodeTest, DecodeSimpleByteArray) {
    // "5:hello" - byte array "hello"
    ByteArray data = {'5', ':', 'h', 'e', 'l', 'l', 'o'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::ByteArray);
    ByteArray expected = {'h', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(result->GetByteArray(), expected);
}

TEST_F(BEncodingDecodeTest, DecodeEmptyByteArray) {
    // "0:" - empty byte array
    ByteArray data = {'0', ':'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::ByteArray);
    EXPECT_TRUE(result->GetByteArray().empty());
}

TEST_F(BEncodingDecodeTest, DecodeByteArrayWithBinaryData) {
    // "3:" followed by 3 arbitrary bytes
    ByteArray data = {'3', ':', 0xFF, 0x00, 0xAB};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::ByteArray);
    ByteArray expected = {0xFF, 0x00, 0xAB};
    EXPECT_EQ(result->GetByteArray(), expected);
}

TEST_F(BEncodingDecodeTest, DecodeByteArrayWithMultiDigitLength) {
    // "10:0123456789" - 10 characters
    std::string str = "10:0123456789";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::ByteArray);
    EXPECT_EQ(result->GetByteArray().size(), 10);
}

// List Decoding Tests
TEST_F(BEncodingDecodeTest, DecodeEmptyList) {
    // "le" - empty list
    ByteArray data = {'l', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::List);
    EXPECT_TRUE(result->GetList().empty());
}

TEST_F(BEncodingDecodeTest, DecodeListWithNumbers) {
    // "li1ei2ei3ee" - list [1, 2, 3]
    ByteArray data = {'l', 'i', '1', 'e', 'i', '2', 'e', 'i', '3', 'e', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::List);
    auto list = result->GetList();
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list[0]->GetNumber(), 1);
    EXPECT_EQ(list[1]->GetNumber(), 2);
    EXPECT_EQ(list[2]->GetNumber(), 3);
}

TEST_F(BEncodingDecodeTest, DecodeListWithStrings) {
    // "l3:one3:two5:threee" - list ["one", "two", "three"]
    std::string str = "l3:one3:two5:threee";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::List);
    auto list = result->GetList();
    EXPECT_EQ(list.size(), 3);
    
    ByteArray expected1 = {'o', 'n', 'e'};
    ByteArray expected2 = {'t', 'w', 'o'};
    ByteArray expected3 = {'t', 'h', 'r', 'e', 'e'};
    
    EXPECT_EQ(list[0]->GetByteArray(), expected1);
    EXPECT_EQ(list[1]->GetByteArray(), expected2);
    EXPECT_EQ(list[2]->GetByteArray(), expected3);
}

TEST_F(BEncodingDecodeTest, DecodeListWithMixedTypes) {
    // "li42e4:spam5:helloe" - list [42, "spam", "hello"]
    std::string str = "li42e4:spam5:helloe";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    auto list = result->GetList();
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list[0]->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(list[0]->GetNumber(), 42);
    EXPECT_EQ(list[1]->GetType(), BEncodedValue::Type::ByteArray);
    EXPECT_EQ(list[2]->GetType(), BEncodedValue::Type::ByteArray);
}

TEST_F(BEncodingDecodeTest, DecodeNestedList) {
    // "lli1ei2eeli3ei4eee" - list [[1, 2], [3, 4]]
    ByteArray data = {'l', 'l', 'i', '1', 'e', 'i', '2', 'e', 'e', 
                      'l', 'i', '3', 'e', 'i', '4', 'e', 'e', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    auto list = result->GetList();
    EXPECT_EQ(list.size(), 2);
    
    auto sublist1 = list[0]->GetList();
    auto sublist2 = list[1]->GetList();
    
    EXPECT_EQ(sublist1.size(), 2);
    EXPECT_EQ(sublist1[0]->GetNumber(), 1);
    EXPECT_EQ(sublist1[1]->GetNumber(), 2);
    
    EXPECT_EQ(sublist2.size(), 2);
    EXPECT_EQ(sublist2[0]->GetNumber(), 3);
    EXPECT_EQ(sublist2[1]->GetNumber(), 4);
}

// Dictionary Decoding Tests
TEST_F(BEncodingDecodeTest, DecodeEmptyDictionary) {
    // "de" - empty dictionary
    ByteArray data = {'d', 'e'};
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Dictionary);
    EXPECT_TRUE(result->GetDictionary().empty());
}

TEST_F(BEncodingDecodeTest, DecodeSimpleDictionary) {
    // "d3:agei25e4:name4:Johne" - {"age": 25, "name": "John"}
    // Note: keys must be sorted by raw bytes
    std::string str = "d3:agei25e4:name4:Johne";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    EXPECT_EQ(result->GetType(), BEncodedValue::Type::Dictionary);
    auto dict = result->GetDictionary();
    
    EXPECT_EQ(dict.size(), 2);
    EXPECT_EQ(dict["age"]->GetNumber(), 25);
    
    ByteArray expectedName = {'J', 'o', 'h', 'n'};
    EXPECT_EQ(dict["name"]->GetByteArray(), expectedName);
}

TEST_F(BEncodingDecodeTest, DecodeDictionaryWithListValue) {
    // "d4:listli1ei2ei3eee" - {"list": [1, 2, 3]}
    std::string str = "d4:listli1ei2ei3eee";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    auto dict = result->GetDictionary();
    EXPECT_EQ(dict.size(), 1);
    EXPECT_EQ(dict["list"]->GetType(), BEncodedValue::Type::List);
    
    auto list = dict["list"]->GetList();
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list[0]->GetNumber(), 1);
    EXPECT_EQ(list[1]->GetNumber(), 2);
    EXPECT_EQ(list[2]->GetNumber(), 3);
}

TEST_F(BEncodingDecodeTest, DecodeNestedDictionary) {
    // "d5:innerd3:keyi42eee" - {"inner": {"key": 42}}
    std::string str = "d5:innerd3:keyi42eee";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    auto dict = result->GetDictionary();
    EXPECT_EQ(dict.size(), 1);
    EXPECT_EQ(dict["inner"]->GetType(), BEncodedValue::Type::Dictionary);
    
    auto innerDict = dict["inner"]->GetDictionary();
    EXPECT_EQ(innerDict.size(), 1);
    EXPECT_EQ(innerDict["key"]->GetNumber(), 42);
}

TEST_F(BEncodingDecodeTest, DecodeDictionaryKeysAreSorted) {
    // Keys should be sorted by raw bytes - "a" (0x61) < "b" (0x62)
    std::string str = "d1:ai1e1:bi2ee";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    auto dict = result->GetDictionary();
    
    EXPECT_EQ(dict["a"]->GetNumber(), 1);
    EXPECT_EQ(dict["b"]->GetNumber(), 2);
}

TEST_F(BEncodingDecodeTest, DecodeDictionaryUnsortedKeysThrowsError) {
    // Keys are not sorted: "b" comes before "a"
    std::string str = "d1:bi2e1:ai1ee";
    ByteArray data = StringToByteArray(str);
    
    EXPECT_THROW({
        BEncoding::Decode(data);
    }, std::runtime_error);
}

// Complex Structure Tests
TEST_F(BEncodingDecodeTest, DecodeComplexNestedStructure) {
    // "d4:listl3:one3:twoe6:numberi42e6:nestedd3:keyi1eee"
    // {"list": ["one", "two"], "number": 42, "nested": {"key": 1}}
    std::string str = "d4:listl3:one3:twoe6:nestedd3:keyi1ee6:numberi42ee";
    ByteArray data = StringToByteArray(str);
    
    auto result = BEncoding::Decode(data);
    
    auto dict = result->GetDictionary();
    EXPECT_EQ(dict.size(), 3);
    
    // Check list
    auto list = dict["list"]->GetList();
    EXPECT_EQ(list.size(), 2);
    
    // Check number
    EXPECT_EQ(dict["number"]->GetNumber(), 42);
    
    // Check nested dict
    auto nested = dict["nested"]->GetDictionary();
    EXPECT_EQ(nested["key"]->GetNumber(), 1);
}

// Error Handling Tests
TEST_F(BEncodingDecodeTest, DecodeInvalidNumberThrowsError) {
    // "iabc e" - invalid number format
    ByteArray data = {'i', 'a', 'b', 'c', 'e'};
    
    EXPECT_THROW({
        BEncoding::Decode(data);
    }, std::exception);
}

TEST_F(BEncodingDecodeTest, DecodeInvalidByteArrayLengthThrowsError) {
    // "abc:hello" - invalid length format
    ByteArray data = {'a', 'b', 'c', ':', 'h', 'e', 'l', 'l', 'o'};
    
    EXPECT_THROW({
        BEncoding::Decode(data);
    }, std::exception);
}

TEST_F(BEncodingDecodeTest, DecodeEmptyDataThrowsError) {
    ByteArray data;
    
    EXPECT_THROW({
        BEncoding::Decode(data);
    }, std::runtime_error);
}

int main(int argc, char **argv){
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
