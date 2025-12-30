#include <gtest/gtest.h>
#include "LitTorrent/BEncoding.h"

using namespace LitTorrent;

class BEncodedValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test data
    }

    void TearDown() override {
        // Cleanup
    }
};

// ByteArray Tests
TEST_F(BEncodedValueTest, CreateByteArrayAndGetType) {
    ByteArray data = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
    auto value = BEncodedValue::CreateByteArray(data);
    
    EXPECT_EQ(value->GetType(), BEncodedValue::Type::ByteArray);
}

TEST_F(BEncodedValueTest, CreateByteArrayAndGetValue) {
    ByteArray data = {0x48, 0x65, 0x6c, 0x6c, 0x6f}; // "Hello"
    auto value = BEncodedValue::CreateByteArray(data);
    
    ByteArray result = value->GetByteArray();
    EXPECT_EQ(result, data);
}

TEST_F(BEncodedValueTest, CreateEmptyByteArray) {
    ByteArray data;
    auto value = BEncodedValue::CreateByteArray(data);
    
    EXPECT_EQ(value->GetType(), BEncodedValue::Type::ByteArray);
    EXPECT_TRUE(value->GetByteArray().empty());
}

// Number Tests
TEST_F(BEncodedValueTest, CreateNumberAndGetType) {
    auto value = BEncodedValue::CreateNumber(42);
    
    EXPECT_EQ(value->GetType(), BEncodedValue::Type::Number);
}

TEST_F(BEncodedValueTest, CreateNumberAndGetValue) {
    int64_t num = 12345;
    auto value = BEncodedValue::CreateNumber(num);
    
    EXPECT_EQ(value->GetNumber(), num);
}

TEST_F(BEncodedValueTest, CreateNegativeNumber) {
    int64_t num = -999;
    auto value = BEncodedValue::CreateNumber(num);
    
    EXPECT_EQ(value->GetNumber(), num);
}

TEST_F(BEncodedValueTest, CreateZeroNumber) {
    auto value = BEncodedValue::CreateNumber(0);
    
    EXPECT_EQ(value->GetNumber(), 0);
}

TEST_F(BEncodedValueTest, CreateLargeNumber) {
    int64_t num = 9223372036854775807LL; // max int64_t
    auto value = BEncodedValue::CreateNumber(num);
    
    EXPECT_EQ(value->GetNumber(), num);
}

// List Tests
TEST_F(BEncodedValueTest, CreateListAndGetType) {
    BEncodedList list;
    auto value = BEncodedValue::CreateList(list);
    
    EXPECT_EQ(value->GetType(), BEncodedValue::Type::List);
}

TEST_F(BEncodedValueTest, CreateEmptyList) {
    BEncodedList list;
    auto value = BEncodedValue::CreateList(list);
    
    EXPECT_TRUE(value->GetList().empty());
}

TEST_F(BEncodedValueTest, CreateListWithElements) {
    BEncodedList list;
    list.push_back(BEncodedValue::CreateNumber(1));
    list.push_back(BEncodedValue::CreateNumber(2));
    list.push_back(BEncodedValue::CreateNumber(3));
    
    auto value = BEncodedValue::CreateList(list);
    auto result = value->GetList();
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]->GetNumber(), 1);
    EXPECT_EQ(result[1]->GetNumber(), 2);
    EXPECT_EQ(result[2]->GetNumber(), 3);
}

TEST_F(BEncodedValueTest, CreateNestedList) {
    BEncodedList innerList;
    innerList.push_back(BEncodedValue::CreateNumber(100));
    
    BEncodedList outerList;
    outerList.push_back(BEncodedValue::CreateList(innerList));
    outerList.push_back(BEncodedValue::CreateNumber(200));
    
    auto value = BEncodedValue::CreateList(outerList);
    auto result = value->GetList();
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0]->GetType(), BEncodedValue::Type::List);
    EXPECT_EQ(result[1]->GetNumber(), 200);
    
    auto nested = result[0]->GetList();
    EXPECT_EQ(nested.size(), 1);
    EXPECT_EQ(nested[0]->GetNumber(), 100);
}

// Dictionary Tests
TEST_F(BEncodedValueTest, CreateDictionaryAndGetType) {
    BEncodedDict dict;
    auto value = BEncodedValue::CreateDictionary(dict);
    
    EXPECT_EQ(value->GetType(), BEncodedValue::Type::Dictionary);
}

TEST_F(BEncodedValueTest, CreateEmptyDictionary) {
    BEncodedDict dict;
    auto value = BEncodedValue::CreateDictionary(dict);
    
    EXPECT_TRUE(value->GetDictionary().empty());
}

TEST_F(BEncodedValueTest, CreateDictionaryWithEntries) {
    BEncodedDict dict;
    dict["age"] = BEncodedValue::CreateNumber(25);
    dict["name"] = BEncodedValue::CreateByteArray(ByteArray{'J', 'o', 'h', 'n'});
    
    auto value = BEncodedValue::CreateDictionary(dict);
    auto result = value->GetDictionary();
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["age"]->GetNumber(), 25);
    
    ByteArray expectedName = {'J', 'o', 'h', 'n'};
    EXPECT_EQ(result["name"]->GetByteArray(), expectedName);
}

TEST_F(BEncodedValueTest, CreateNestedDictionary) {
    BEncodedDict innerDict;
    innerDict["inner_key"] = BEncodedValue::CreateNumber(42);
    
    BEncodedDict outerDict;
    outerDict["nested"] = BEncodedValue::CreateDictionary(innerDict);
    outerDict["value"] = BEncodedValue::CreateNumber(100);
    
    auto value = BEncodedValue::CreateDictionary(outerDict);
    auto result = value->GetDictionary();
    
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result["nested"]->GetType(), BEncodedValue::Type::Dictionary);
    EXPECT_EQ(result["value"]->GetNumber(), 100);
    
    auto nested = result["nested"]->GetDictionary();
    EXPECT_EQ(nested["inner_key"]->GetNumber(), 42);
}

TEST_F(BEncodedValueTest, CreateMixedTypeList) {
    BEncodedList list;
    list.push_back(BEncodedValue::CreateNumber(42));
    list.push_back(BEncodedValue::CreateByteArray(ByteArray{'t', 'e', 's', 't'}));
    
    BEncodedDict dict;
    dict["key"] = BEncodedValue::CreateNumber(1);
    list.push_back(BEncodedValue::CreateDictionary(dict));
    
    auto value = BEncodedValue::CreateList(list);
    auto result = value->GetList();
    
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0]->GetType(), BEncodedValue::Type::Number);
    EXPECT_EQ(result[1]->GetType(), BEncodedValue::Type::ByteArray);
    EXPECT_EQ(result[2]->GetType(), BEncodedValue::Type::Dictionary);
}

int main(int argc, char **argv){
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
