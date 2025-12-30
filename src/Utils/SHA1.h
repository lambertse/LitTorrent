#pragma once

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

class SHA1 {
public:
  static std::string ComputeHash(const std::string &input) {
    // Initialize hash values
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // Prepare message
    std::string msg = input;
    size_t originalBits = msg.size() * 8;

    // Append padding
    msg += (char)0x80;
    while ((msg.size() % 64) != 56) {
      msg += (char)0x00;
    }

    // Append length (64-bit big-endian)
    for (int i = 7; i >= 0; i--) {
      msg += (char)((originalBits >> (i * 8)) & 0xFF);
    }

    // Process each 512-bit block
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
      uint32_t w[80];

      // Break chunk into sixteen 32-bit big-endian words
      for (int i = 0; i < 16; i++) {
        w[i] = ((unsigned char)msg[chunk + i * 4] << 24) |
               ((unsigned char)msg[chunk + i * 4 + 1] << 16) |
               ((unsigned char)msg[chunk + i * 4 + 2] << 8) |
               ((unsigned char)msg[chunk + i * 4 + 3]);
      }

      // Extend the sixteen 32-bit words into eighty 32-bit words
      for (int i = 16; i < 80; i++) {
        uint32_t temp = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
        w[i] = (temp << 1) | (temp >> 31);
      }

      // Initialize working variables
      uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

      // Main loop
      for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
          f = (b & c) | (~b & d);
          k = 0x5A827999;
        } else if (i < 40) {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
        } else if (i < 60) {
          f = (b & c) | (b & d) | (c & d);
          k = 0x8F1BBCDC;
        } else {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
        }

        uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
        e = d;
        d = c;
        c = (b << 30) | (b >> 2);
        b = a;
        a = temp;
      }

      // Add this chunk's hash to result
      h0 += a;
      h1 += b;
      h2 += c;
      h3 += d;
      h4 += e;
    }

    // Convert to hex string
    std::ostringstream result;
    result << std::hex << std::setfill('0') << std::setw(8) << h0
           << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
           << std::setw(8) << h4;

    return result.str();
  }
};
