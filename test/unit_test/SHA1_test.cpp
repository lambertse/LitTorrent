#include <gtest/gtest.h>
#include "../src/Utils/SHA1.h"

class SHA1Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

// Test empty string
TEST_F(SHA1Test, EmptyString) {
    std::string hash = SHA1::computeHash("");
    EXPECT_EQ(hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

// Test single character
TEST_F(SHA1Test, SingleCharacter) {
    std::string hash = SHA1::computeHash("a");
    EXPECT_EQ(hash, "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");
}

// Test common phrase
TEST_F(SHA1Test, QuickBrownFox) {
    std::string hash = SHA1::computeHash("The quick brown fox jumps over the lazy dog");
    EXPECT_EQ(hash, "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

// Test with period at end
TEST_F(SHA1Test, QuickBrownFoxWithPeriod) {
    std::string hash = SHA1::computeHash("The quick brown fox jumps over the lazy dog.");
    EXPECT_EQ(hash, "408d94384216f890ff7a0c3528e8bed1e0b01621");
}

// Test simple strings
TEST_F(SHA1Test, SimpleABC) {
    std::string hash = SHA1::computeHash("abc");
    EXPECT_EQ(hash, "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST_F(SHA1Test, SimpleMessage) {
    std::string hash = SHA1::computeHash("message digest");
    EXPECT_EQ(hash, "c12252ceda8be8994d5fa0290a47231c1d16aae3");
}

// Test alphabet
TEST_F(SHA1Test, Alphabet) {
    std::string hash = SHA1::computeHash("abcdefghijklmnopqrstuvwxyz");
    EXPECT_EQ(hash, "32d10c7b8cf96570ca04ce37f2a19d84240d3a89");
}

// Test alphanumeric
TEST_F(SHA1Test, Alphanumeric) {
    std::string hash = SHA1::computeHash("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    EXPECT_EQ(hash, "761c457bf73b14d27e9e9265c46f4b4dda11f940");
}

// Test repeated digits
TEST_F(SHA1Test, RepeatedDigits) {
    std::string hash = SHA1::computeHash("12345678901234567890123456789012345678901234567890123456789012345678901234567890");
    EXPECT_EQ(hash, "50abf5706a150990a08b2c5ea40fa0e585554732");
}

// Test longer message
TEST_F(SHA1Test, LongMessage) {
    std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    std::string hash = SHA1::computeHash(input);
    EXPECT_EQ(hash, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

// Test numeric string
TEST_F(SHA1Test, NumericString) {
    std::string hash = SHA1::computeHash("123456");
    EXPECT_EQ(hash, "7c4a8d09ca3762af61e59520943dc26494f8941b");
}

// Test with whitespace
TEST_F(SHA1Test, WithWhitespace) {
    std::string hash = SHA1::computeHash("Hello World");
    EXPECT_EQ(hash, "0a4d55a8d778e5022fab701977c5d840bbc486d0");
}

// Test with newline - CORRECTED
TEST_F(SHA1Test, WithNewline) {
    std::string hash = SHA1::computeHash("Hello\nWorld");
    EXPECT_EQ(hash, "978d47f77be4b032782af0e30066ee1a285f55d9");
}

// Test with special characters - CORRECTED
TEST_F(SHA1Test, SpecialCharacters) {
    std::string hash = SHA1::computeHash("!@#$%^&*()");
    EXPECT_EQ(hash, "bf24d65c9bb05b9b814a966940bcfa50767c8a8d");
}

// Test UTF-8 characters (basic ASCII range) - CORRECTED
TEST_F(SHA1Test, Numbers) {
    std::string hash = SHA1::computeHash("0123456789");
    EXPECT_EQ(hash, "87acec17cd9dcd20a716cc2cf67417b71c8a7016");
}

// Test case sensitivity
TEST_F(SHA1Test, CaseSensitive) {
    std::string hash1 = SHA1::computeHash("Hello");
    std::string hash2 = SHA1::computeHash("hello");
    EXPECT_NE(hash1, hash2);
    EXPECT_EQ(hash1, "f7ff9e8b7bb2e09b70935a5d785e0cc5d9d0abf0");
    EXPECT_EQ(hash2, "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d");
}

// Test repeated string - CORRECTED
TEST_F(SHA1Test, RepeatedA) {
    std::string hash = SHA1::computeHash("aaaaaaaaaa");
    EXPECT_EQ(hash, "3495ff69d34671d1e15b33a63c1379fdedd3a32a");
}

// Test hash consistency (calling multiple times)
TEST_F(SHA1Test, Consistency) {
    std::string input = "test";
    std::string hash1 = SHA1::computeHash(input);
    std::string hash2 = SHA1::computeHash(input);
    std::string hash3 = SHA1::computeHash(input);
    
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
    EXPECT_EQ(hash1, "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3");
}

// Test output format (should be 40 hex characters)
TEST_F(SHA1Test, OutputFormat) {
    std::string hash = SHA1::computeHash("test");
    EXPECT_EQ(hash.length(), 40);
    
    // Check all characters are valid hex
    for (char c : hash) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

// Additional edge case tests
TEST_F(SHA1Test, SingleSpace) {
    std::string hash = SHA1::computeHash(" ");
    EXPECT_EQ(hash.length(), 40);
}

TEST_F(SHA1Test, TabCharacter) {
    std::string hash = SHA1::computeHash("\t");
    EXPECT_EQ(hash.length(), 40);
}

TEST_F(SHA1Test, VeryLongString) {
    std::string longString(1000, 'x');
    std::string hash = SHA1::computeHash(longString);
    EXPECT_EQ(hash.length(), 40);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
