#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

namespace qrcraft {
using namespace std; // Isolated to qrcraft namespace

// --- BEGIN VersionUtils.h ---

inline int getEclIndex(char ecl) {
    if (ecl == 'L') return 0;
    if (ecl == 'M') return 1;
    if (ecl == 'Q') return 2;
    if (ecl == 'H') return 3;
    return 0; // default L
}

inline int getMaxDataBits(int version, char ecl){
    // Max data bits for QR Version 1 to 40, Error Correction Levels L, M, Q, H
    static const int maxBits[4][41] = {
        {-1, 152, 272, 440, 640, 864, 1088, 1248, 1552, 1856, 2192, 2592, 2960, 3424, 3688, 4184, 4712, 5176, 5768, 6360, 6888, 7456, 8048, 8752, 9392, 10208, 10960, 11744, 12248, 13048, 13880, 14744, 15640, 16568, 17528, 18448, 19472, 20528, 21616, 22496, 23648},
        {-1, 128, 224, 352, 512, 688, 864, 992, 1232, 1456, 1728, 2032, 2320, 2672, 2920, 3320, 3624, 4056, 4504, 5016, 5352, 5712, 6256, 6880, 7312, 8000, 8496, 9024, 9544, 10136, 10984, 11640, 12328, 13048, 13800, 14496, 15312, 15936, 16816, 17728, 18672},
        {-1, 104, 176, 272, 384, 496, 608, 704, 880, 1056, 1232, 1440, 1648, 1952, 2088, 2360, 2600, 2936, 3176, 3560, 3880, 4096, 4544, 4912, 5312, 5744, 6032, 6464, 6968, 7288, 7880, 8264, 8920, 9368, 9848, 10288, 10832, 11408, 12016, 12656, 13328},
        {-1, 72, 128, 208, 288, 368, 480, 528, 688, 800, 976, 1120, 1264, 1440, 1576, 1784, 2024, 2264, 2504, 2728, 3080, 3248, 3536, 3712, 4112, 4304, 4768, 5024, 5288, 5608, 5960, 6344, 6760, 7208, 7688, 7888, 8432, 8768, 9136, 9776, 10208}
    };

    if(version < 1 || version > 40) return -1;
    return maxBits[getEclIndex(ecl)][version];
}

inline int decideVersion(const std::vector<bool>& encodedBits, char ecl){
    for(int version = 1; version <= 40; version++){
        if(encodedBits.size() <= getMaxDataBits(version, ecl))
            return version;
    }
    return -1; // Data too big for version 40 at specified ECL
}
// --- END VersionUtils.h ---

// --- BEGIN ModeEncoder.h ---

class ModeEncoder{  // interface for all encoding modes
public:
    virtual bool canEncode(const string& data) = 0;
    virtual vector<bool> encode(const string& data, char ecl) = 0;
    virtual int getCharacterCountBits(int version) = 0;
    virtual ~ModeEncoder() {}
};// --- END ModeEncoder.h ---

// --- BEGIN NumericEncoder.h ---

class NumericEncoder : public ModeEncoder{
public:
    bool canEncode(const string& data){
        for(char c : data)
            if(!isdigit(c)) return false;
        return true; 
    }

    vector<bool> encode(const string& data, char ecl) {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator: 0001
        bits.push_back(0); bits.push_back(0); bits.push_back(0); bits.push_back(1);

        int version = 1;
        for (; version <= 40; ++version) {
            int total = 4 + getCharacterCountBits(version) + (int)rawBits.size();
            if (total <= getMaxDataBits(version, ecl))
                break;
        }
        int characterBits = getCharacterCountBits(version);

        // Character count bits
        int len = data.size();
        for (int i = characterBits - 1; i >= 0; i--)
            bits.push_back((len >> i) & 1);

        // encoding the the data string into the bits 
        // for the group of 3 chars : required bits = 10
        // for the group of 2 chars : required bits = 7
        // for the group of single char : required bits = 4
        // all the values of groups of 3 chars, 2 chars and single char are corresponding integer value
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 3) {
                int value = (data[i] - '0') * 100 + (data[i + 1] - '0') * 10 + (data[i + 2] - '0');
                for (int j = 9; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 3;
            }
            else if (remaining == 2) {
                int value = (data[i] - '0') * 10 + (data[i + 1] - '0');
                for (int j = 6; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 2;
            }
            else { // remaining == 1
                int value = data[i] - '0';
                for (int j = 3; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 1;
            }
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int i = 0, len = data.size();
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 3) {
                int value = (data[i] - '0') * 100 + (data[i + 1] - '0') * 10 + (data[i + 2] - '0');
                for (int j = 9; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 3;
            }
            else if (remaining == 2) {
                int value = (data[i] - '0') * 10 + (data[i + 1] - '0');
                for (int j = 6; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 2;
            }
            else { // remaining == 1
                int value = data[i] - '0';
                for (int j = 3; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 1;
            }
        }

        return bits;
    }

    int getCharacterCountBits(int version){
        if(version >= 1 && version <= 9) return 10;
        else if(version <= 26) return 12;
        else return 14;
    }
};// --- END NumericEncoder.h ---

// --- BEGIN AlphanumericEncoder.h ---


class AlphanumericEncoder : public ModeEncoder {
    unordered_map<char, int> table = {
        {'0',0},{'1',1},{'2',2},{'3',3},{'4',4},{'5',5},{'6',6},{'7',7},{'8',8},{'9',9},
        {'A',10},{'B',11},{'C',12},{'D',13},{'E',14},{'F',15},{'G',16},{'H',17},{'I',18},{'J',19},
        {'K',20},{'L',21},{'M',22},{'N',23},{'O',24},{'P',25},{'Q',26},{'R',27},{'S',28},{'T',29},
        {'U',30},{'V',31},{'W',32},{'X',33},{'Y',34},{'Z',35},{' ',36},{'$',37},{'%',38},{'*',39},
        {'+',40},{'-',41},{'.',42},{'/',43},{':',44}
    };
public:
    bool canEncode(const string &data){
        for (char c : data)
            if (table.find(c) == table.end()) return false;
        return true;
    }

    vector<bool> encode(const string& data, char ecl) {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator: 0010
        bits.push_back(0); bits.push_back(0); bits.push_back(1); bits.push_back(0);

        int version = 1;
        for (; version <= 40; ++version) {
            int total = 4 + getCharacterCountBits(version) + (int)rawBits.size();
            if (total <= getMaxDataBits(version, ecl))
                break;
        }
        int characterBits = getCharacterCountBits(version);

        // Character count bits
        int len = data.size();
        for (int i = characterBits - 1; i >= 0; i--)
            bits.push_back((len >> i) & 1);

        // encoding all the input into the bits with respect to the encoding rule for the alphanumeric string
        // for the group of two : 11 bits required and the value is appended
        // for only one char : 6 bits required and the value is appended
        // value (for group of two) = value of first char from map * 45 + value of second char from the map
        // value (for single char) = corresponding value of the char from the map
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 2) {
                int value = table[data[i]] * 45 + table[data[i + 1]];
                for (int j = 10; j >= 0; j--) bits.push_back((value >> j) & 1); // 11 bits
                i += 2;
            }
            else { // remaining == 1
                int value = table[data[i]];
                for (int j = 5; j >= 0; j--) bits.push_back((value >> j) & 1); // 6 bits
                i += 1;
            }
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int len = data.size();
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 2) {
                int value = table[data[i]] * 45 + table[data[i + 1]];
                for (int j = 10; j >= 0; j--) bits.push_back((value >> j) & 1); // 11 bits
                i += 2;
            }
            else { // remaining == 1
                int value = table[data[i]];
                for (int j = 5; j >= 0; j--) bits.push_back((value >> j) & 1); // 6 bits
                i += 1;
            }
        }

        return bits;
    }

    int getCharacterCountBits(int version){
        if (version >= 1 && version <= 9) return 9;
        else if (version <= 26) return 11;
        else return 13;
    }
};// --- END AlphanumericEncoder.h ---

// --- BEGIN ByteEncoder.h ---


class ByteEncoder : public ModeEncoder{
public:
    bool canEncode(const string &data) override {
        return true;
    }

    vector<bool> encode(const string& data, char ecl) override {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator (4 bits for byte mode)
        bits.push_back(0); bits.push_back(1); bits.push_back(0); bits.push_back(0);

        // Version for count field: smallest v where mode + count + payload fits
        int version = 1;
        for (; version <= 40; ++version) {
            int total = 4 + getCharacterCountBits(version) + (int)rawBits.size();
            if (total <= getMaxDataBits(version, ecl))
                break;
        }

        int characterBits = getCharacterCountBits(version);
        int len = data.size();
        for (int i = characterBits - 1; i >= 0; --i)
            bits.push_back((len >> i) & 1);

        // Encode each character in 8-bit ASCII
        for (unsigned char c : data) {
            for (int i = 7; i >= 0; --i)
                bits.push_back((c >> i) & 1);
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int len = data.size();

        // Encode each character in 8-bit ASCII
        for (unsigned char c : data) {
            for (int i = 7; i >= 0; --i)
                bits.push_back((c >> i) & 1);
        }

        return bits;
    }

    int getCharacterCountBits(int version) override {
        if (version >= 1 && version <= 9) return 8;
        else return 16;
    }
};// --- END ByteEncoder.h ---

// --- BEGIN PaddingUtils.h ---

inline void applyTerminatorAndPadding(std::vector<bool>& bits, int version, char ecl){
    int capacity = getMaxDataBits(version, ecl);

    // terminator
    int remaining = capacity - bits.size();
    int terminator = (remaining >= 4) ? 4 : remaining;
    for(int i = 0; i < terminator; i++)
        bits.push_back(0);

    // alignment of byte
    while(bits.size() % 8 != 0)
        bits.push_back(0);

    // pad bytes
    bool toggle = true;
    while(bits.size() < capacity){
        unsigned char pad = toggle ? 0xEC : 0x11;
        toggle = !toggle;

        for(int i = 7; i >= 0; i--)
            bits.push_back((pad >> i) & 1);
    }
}// --- END PaddingUtils.h ---

// --- BEGIN CodewordUtils.h ---

inline vector<unsigned char> bitsToCodeWords(const vector<bool>& bits){
    vector<unsigned char> codewords;

    // here size_t is used because it will guaranteed to represent the size of any object in memory
    // the .size() function also returns the size of type size_t
    for(size_t i = 0; i < bits.size(); i += 8){
        unsigned char byte = 0;

        for(int j = 0; j < 8; j++){
            byte <<= 1;
            byte |= bits[i + j];
        }

        codewords.push_back(byte);
    }

    return codewords;
}// --- END CodewordUtils.h ---

// --- BEGIN ErrorCorrection.h ---


namespace qrmath {
namespace ErrorCorrection {

// GF(2^8) mod x^8 + x^4 + x^3 + x^2 + 1 (0x11D) — same as ISO/IEC 18004.
inline unsigned char reedSolomonMultiply(uint8_t x, uint8_t y) {
    int z = 0;
    for (int i = 7; i >= 0; i--) {
        z = (z << 1) ^ ((z >> 7) * 0x11D);
        z ^= ((y >> i) & 1) * x;
    }
    return static_cast<unsigned char>(z);
}

inline unsigned char multiplyGF(unsigned char a, unsigned char b) {
    return reedSolomonMultiply(static_cast<uint8_t>(a), static_cast<uint8_t>(b));
}

inline unsigned char gfPow(unsigned char a, int e) {
    unsigned char r = 1;
    while (e-- > 0) r = reedSolomonMultiply(r, a);
    return r;
}

// Generator ∏_{i=0}^{degree-1} (x - α^i), α = 0x02; coeffs high→low excluding x^degree term.
inline std::vector<unsigned char> reedSolomonComputeDivisor(int degree) {
    std::vector<unsigned char> result(static_cast<size_t>(degree));
    result[result.size() - 1] = 1;
    uint8_t root = 1;
    for (int i = 0; i < degree; i++) {
        for (size_t j = 0; j < result.size(); j++) {
            result[j] = reedSolomonMultiply(result[j], root);
            if (j + 1 < result.size())
                result[j] ^= result[j + 1];
        }
        root = reedSolomonMultiply(root, 2);
    }
    return result;
}

inline std::vector<unsigned char> reedSolomonComputeRemainder(
    const std::vector<unsigned char>& data,
    const std::vector<unsigned char>& divisor) {
    std::vector<unsigned char> result(divisor.size(), 0);
    for (uint8_t b : data) {
        uint8_t factor = static_cast<uint8_t>(b ^ result[0]);
        result.erase(result.begin());
        result.push_back(0);
        for (size_t i = 0; i < result.size(); i++)
            result[i] ^= reedSolomonMultiply(divisor[i], factor);
    }
    return result;
}

inline std::vector<unsigned char> computeECCCodewords(
    const std::vector<unsigned char>& data,
    int eccCount) {
    std::vector<unsigned char> divisor = reedSolomonComputeDivisor(eccCount);
    return reedSolomonComputeRemainder(data, divisor);
}

// Exposed for tests / debugging (product (x-α^0)...(x-α^{n-1}) expanded).
inline std::vector<unsigned char> generateGeneratorPoly(int n) {
    return reedSolomonComputeDivisor(n);
}

} // namespace ErrorCorrection
} // namespace QRCode
// --- END ErrorCorrection.h ---

// --- BEGIN VersionBlocks.h ---


// ISO/IEC 18004 block structure for error levels L, M, Q, H (index 1..40; 0 unused).
// Row indices: 0 = L, 1 = M, 2 = Q, 3 = H

static const int8_t ECC_CODEWORDS_PER_BLOCK[4][41] = {
    {-1,  7, 10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28, 28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {-1, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26, 30, 22, 22, 24, 24, 28, 28, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28},
    {-1, 13, 22, 18, 26, 18, 24, 18, 22, 20, 24, 28, 26, 24, 20, 30, 24, 28, 28, 26, 30, 28, 30, 30, 30, 30, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30},
    {-1, 17, 28, 22, 16, 22, 28, 26, 26, 24, 28, 24, 28, 22, 24, 24, 30, 28, 28, 26, 28, 30, 24, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30}
};

static const int8_t NUM_ERROR_CORRECTION_BLOCKS[4][41] = {
    {-1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,  8,  9,  9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25},
    {-1, 1, 1, 1, 2, 2, 4, 4, 4, 5, 5,  5,  8,  9,  9, 10, 10, 11, 13, 14, 16, 17, 17, 18, 20, 21, 23, 25, 26, 28, 29, 31, 33, 35, 37, 38, 40, 43, 45, 47, 49},
    {-1, 1, 1, 2, 2, 4, 4, 6, 6, 8, 8,  8, 10, 12, 16, 12, 17, 16, 18, 21, 20, 23, 23, 25, 27, 29, 34, 34, 35, 38, 40, 43, 45, 48, 51, 53, 56, 59, 62, 65, 68},
    {-1, 1, 1, 2, 4, 4, 4, 5, 6, 8, 8, 11, 11, 16, 16, 18, 16, 19, 21, 25, 25, 25, 34, 30, 32, 35, 37, 40, 42, 45, 48, 51, 54, 57, 60, 63, 66, 70, 74, 77, 81}
};

struct BlockInfo {
    int numDataBlocks;
    int numEccBlocks;
    int dataCwPerBlock;
    int eccCwPerBlock;
};

inline BlockInfo getBlockInfo(int version, char ecl) {
    if (version < 1 || version > 40)
        return {0, 0, 0, 0};
    int idx = getEclIndex(ecl);
    int nb = NUM_ERROR_CORRECTION_BLOCKS[idx][version];
    int ecc = ECC_CODEWORDS_PER_BLOCK[idx][version];
    int dataTotal = getMaxDataBits(version, ecl) / 8;
    return {nb, nb, (dataTotal + nb - 1) / nb, ecc};
}

inline int getTotalDataBits(int version, char ecl) {
    return getMaxDataBits(version, ecl);
}

inline int getTotalCodewords(int version, char ecl) {
    int data = getMaxDataBits(version, ecl) / 8;
    int idx = getEclIndex(ecl);
    int nb = NUM_ERROR_CORRECTION_BLOCKS[idx][version];
    int ecc = ECC_CODEWORDS_PER_BLOCK[idx][version];
    return data + nb * ecc;
}

// Split padded data codewords into RS blocks, append ECC, interleave (ISO/IEC 18004).
inline std::vector<unsigned char> buildInterleavedCodewords(
    const std::vector<unsigned char>& dataCodewords,
    int version,
    char ecl
) {
    assert(version >= 1 && version <= 40);
    int idx = getEclIndex(ecl);

    const int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[idx][version];
    const int blockEccLen = ECC_CODEWORDS_PER_BLOCK[idx][version];
    const int dataCapacity = getMaxDataBits(version, ecl) / 8;
    assert(static_cast<int>(dataCodewords.size()) == dataCapacity);

    const int rawCodewords = dataCapacity + numBlocks * blockEccLen;
    const int numShortBlocks = numBlocks - (rawCodewords % numBlocks);
    const int shortBlockLen = rawCodewords / numBlocks;

    std::vector<std::vector<unsigned char>> blocks;
    int k = 0;
    for (int i = 0; i < numBlocks; ++i) {
        const int dataLen = shortBlockLen - blockEccLen + (i < numShortBlocks ? 0 : 1);
        std::vector<unsigned char> dat(
            dataCodewords.begin() + k,
            dataCodewords.begin() + k + dataLen
        );
        k += dataLen;

        std::vector<unsigned char> ecc =
            qrmath::ErrorCorrection::computeECCCodewords(dat, blockEccLen);
        if (i < numShortBlocks)
            dat.push_back(0);
        dat.insert(dat.end(), ecc.begin(), ecc.end());
        blocks.push_back(std::move(dat));
    }

    std::vector<unsigned char> result;
    result.reserve(static_cast<size_t>(rawCodewords));
    const size_t rowLen = blocks[0].size();
    const size_t skipIndex = static_cast<size_t>(shortBlockLen - blockEccLen);
    for (size_t i = 0; i < rowLen; ++i) {
        for (size_t j = 0; j < blocks.size(); ++j) {
            if (i != skipIndex || static_cast<int>(j) >= numShortBlocks)
                result.push_back(blocks[j][i]);
        }
    }
    assert(static_cast<int>(result.size()) == rawCodewords);
    return result;
}
// --- END VersionBlocks.h ---

// --- BEGIN DataInterleaving.h ---

/*
    Interleave data and ECC blocks according to QR standard.
    This ensures robustness: damage in one area affects both data & ECC in all blocks.
*/

inline vector<unsigned char> interleaveBlocks(
    const vector<vector<unsigned char>>& dataBlocks,
    const vector<vector<unsigned char>>& eccBlocks
){
    vector<unsigned char> result;

    // finding the max block length/size
    size_t maxDataLen = 0;
    for(const vector<unsigned char>& block : dataBlocks)
        if(block.size() > maxDataLen) maxDataLen = block.size();

    // interleave the data bytes first
    for(size_t i = 0; i < maxDataLen; i++){
        // appending the ith byte from each block of data
        for(const vector<unsigned char>& block : dataBlocks){
            if(i < block.size())
                result.push_back(block[i]);
        }
    }

    // interleaving ecc bytes
    size_t maxEccLen = 0;
    for(const vector<unsigned char>& block : eccBlocks)
        if(block.size() > maxEccLen) maxEccLen = block.size();

    for(size_t i = 0; i < maxEccLen; i++){
        for(const vector<unsigned char>& block : eccBlocks){
            if(i < block.size())
                result.push_back(block[i]);
        }
    }

    return result;
}// --- END DataInterleaving.h ---

// --- BEGIN MatrixUtils.h ---


/*
  Conventions:
  - matrix[r][c]: -1 empty, 0 white, 1 black
  - isFunction[r][c]: true if fixed/reserved (finder, separator, timing, alignment, dark module, format, version)
*/

struct QRGrid {
    vector<vector<int>> matrix;
    vector<vector<bool>> isFunction;
};

/*==================================================
  BASIC UTILITIES
==================================================*/

inline int qrSize(int version) {
    return 21 + 4 * (version - 1);
}

/*==================================================
  ALIGNMENT PATTERN TABLE (ISO/IEC 18004)
==================================================*/

inline const vector<vector<int>>& alignmentTable() {
    static const vector<vector<int>> tbl = {
        {},                         // v1
        {6,18},{6,22},{6,26},{6,30},{6,34},                 // v2..v6
        {6,22,38},{6,24,42},{6,26,46},{6,28,50},            // v7..v10
        {6,30,54},{6,32,58},{6,34,62},                      // v11..v13
        {6,26,46,66},{6,26,48,70},{6,26,50,74},             // v14..v16
        {6,30,54,78},{6,30,56,82},{6,30,58,86},{6,34,62,90},// v17..v20
        {6,28,50,72,94},{6,26,50,74,98},{6,30,54,78,102},   // v21..v23
        {6,28,54,80,106},{6,32,58,84,110},{6,30,58,86,114}, // v24..v26
        {6,34,62,90,118},{6,26,50,74,98,122},{6,30,54,78,102,126}, // v27..v29
        {6,26,52,78,104,130},{6,30,56,82,108,134},{6,34,60,86,112,138}, // v30..v32
        {6,30,58,86,114,142},{6,34,62,90,118,146},          // v33..v34
        {6,30,54,78,102,126,150},{6,24,50,76,102,128,154},  // v35..v36
        {6,28,54,80,106,132,158},{6,32,58,84,110,136,162},  // v37..v38
        {6,26,54,82,110,138,166},{6,30,58,86,114,142,170}   // v39..v40
    };
    return tbl;
}

/*==================================================
  FUNCTION MODULE PLACEMENT
==================================================*/

inline void mark(QRGrid& qr, int r, int c, int v) {
    int n = (int)qr.matrix.size();
    if (r < 0 || c < 0 || r >= n || c >= n) return;
    qr.matrix[r][c] = v;
    qr.isFunction[r][c] = true;
}

// Finder + separator combined: place 7x7 finder and 1-cell white border
inline void placeFinderWithSeparator(QRGrid& qr, int r0, int c0) {
    // Finder pattern
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            bool border = (r == 0 || r == 6 || c == 0 || c == 6);
            bool ring   = (r == 1 || r == 5 || c == 1 || c == 5);
            bool center = (r >= 2 && r <= 4 && c >= 2 && c <= 4);
            int val = border ? 1 : (ring ? 0 : (center ? 1 : 0));
            mark(qr, r0 + r, c0 + c, val);
        }
    }
    // Separator (one-cell white border)
    for (int r = -1; r <= 7; r++) {
        for (int c = -1; c <= 7; c++) {
            if (r >= 0 && r <= 6 && c >= 0 && c <= 6) continue; // skip finder area
            mark(qr, r0 + r, c0 + c, 0);
        }
    }
}

// Check if a 5x5 alignment region is fully within bounds and non-overlapping
inline bool alignmentRegionIsFree(const QRGrid& qr, int r0, int c0) {
    int n = (int)qr.matrix.size();
    for (int r = -2; r <= 2; r++) {
        for (int c = -2; c <= 2; c++) {
            int rr = r0 + r, cc = c0 + c;
            if (rr < 0 || cc < 0 || rr >= n || cc >= n) return false;
            if (qr.isFunction[rr][cc]) return false;
        }
    }
    return true;
}

// 5x5 alignment pattern centered at (r0,c0)
inline void placeAlignmentIfFree(QRGrid& qr, int r0, int c0) {
    if (!alignmentRegionIsFree(qr, r0, c0)) return;
    for (int r = -2; r <= 2; r++) {
        for (int c = -2; c <= 2; c++) {
            int rr = r0 + r, cc = c0 + c;
            int v = (abs(r) == 2 || abs(c) == 2 || (r == 0 && c == 0)) ? 1 : 0;
            mark(qr, rr, cc, v);
        }
    }
}

// Timing patterns (row 6 & column 6)
inline void placeTiming(QRGrid& qr) {
    int n = (int)qr.matrix.size();
    for (int i = 0; i < n; i++) {
        int bit = (i % 2 == 0) ? 1 : 0;
        if (!qr.isFunction[6][i]) mark(qr, 6, i, bit);
        if (!qr.isFunction[i][6]) mark(qr, i, 6, bit);
    }
}

// Dark module
inline void placeDarkModule(QRGrid& qr, int version) {
    int r = 4 * version + 9;
    if (r >= 0 && r < (int)qr.matrix.size()) mark(qr, r, 8, 1);
}

// Reserve format information cells (do not write bits yet)
inline void reserveFormatInfo(QRGrid& qr) {
    int n = (int)qr.matrix.size();
    // Top-left L: row 8 (c=0..8 except 6), col 8 (r=0..8 except 6)
    for (int c = 0; c <= 8; c++) if (c != 6) qr.isFunction[8][c] = true;
    for (int r = 0; r <= 8; r++) if (r != 6) qr.isFunction[r][8] = true;
    // Second vertical copy: column 8, rows (n-15+8)..(n-1) — bits 8..14 of format (ISO / qrcode)
    for (int i = 8; i < 15; ++i) {
        int r = n - 15 + i;
        if (r >= 0 && r < n) qr.isFunction[r][8] = true;
    }
    // Other set: column n-8 (rows 0..6), row 8 (cols n-8..n-1)
    for (int r = 0; r <= 6; r++) qr.isFunction[r][n - 8] = true;
    for (int c = n - 8; c < n; c++) qr.isFunction[8][c] = true;
}

// Reserve version info cells for version >= 7 (do not write bits yet)
inline void reserveVersionInfo(QRGrid& qr, int version) {
    if (version < 7) return;
    int n = (int)qr.matrix.size();
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 3; c++) {
            qr.isFunction[r][n - 11 + c] = true; // top-right
            qr.isFunction[n - 11 + c][r] = true; // bottom-left
        }
    }
}

/*==================================================
  INITIALIZATION (versions 1..40)
==================================================*/

inline QRGrid initializeQRMatrix(int version) {
    int n = qrSize(version);
    QRGrid qr;
    qr.matrix.assign(n, vector<int>(n, -1));
    qr.isFunction.assign(n, vector<bool>(n, false));

    // Finders + separators
    placeFinderWithSeparator(qr, 0, 0);
    placeFinderWithSeparator(qr, 0, n - 7);
    placeFinderWithSeparator(qr, n - 7, 0);

    // Timing
    placeTiming(qr);

    // Alignment patterns
    const auto& centers = alignmentTable()[version - 1];
    if (!centers.empty()) {
        for (int r : centers) {
            for (int c : centers) {
                // Finder centers are already reserved; placement skips due to overlap check
                placeAlignmentIfFree(qr, r, c);
            }
        }
    }

    // Dark module
    placeDarkModule(qr, version);

    // Reserve format and version info
    reserveFormatInfo(qr);
    reserveVersionInfo(qr, version);

    return qr;
}

/*==================================================
  DATA PLACEMENT (ZIG-ZAG, skip reserved)
==================================================*/

inline void placeDataBits(QRGrid& qr, const vector<unsigned char>& codewords) {
    int n = (int)qr.matrix.size();
    int byte = 0, bit = 7;
    bool up = true;

    auto readBit = [&](int& out) -> bool {
        if (byte >= (int)codewords.size()) return false;
        out = (codewords[byte] >> bit) & 1;
        if (--bit < 0) { bit = 7; ++byte; }
        return true;
    };

    // Mirror qrcode/main.py map_data: outer step is ncol -= 2; only then apply col <= 6 ? col-- for this pair
    for (int ncol = n - 1; ncol > 0; ncol -= 2) {
        int col = ncol;
        if (col <= 6)
            col--;
        for (int i = 0; i < n; i++) {
            int row = up ? (n - 1 - i) : i;
            for (int k = 0; k < 2; k++) {
                int c = col - k;
                if (c < 0) continue;
                if (!qr.isFunction[row][c]) {
                    int b = 0;
                    readBit(b);
                    qr.matrix[row][c] = b;
                }
            }
        }
        up = !up;
    }

}

/*==================================================
  MASKS AND PENALTY SCORING
==================================================*/

inline int maskFn(int p, int r, int c) {
    switch (p) {
        case 0: return ((r + c) % 2 == 0);
        case 1: return (r % 2 == 0);
        case 2: return (c % 3 == 0);
        case 3: return ((r + c) % 3 == 0);
        case 4: return (((r / 2) + (c / 3)) % 2 == 0);
        case 5: return (((r * c) % 2) + ((r * c) % 3) == 0);
        case 6: return ((((r * c) % 2) + ((r * c) % 3)) % 2 == 0);
        case 7: return ((((r + c) % 2) + ((r * c) % 3)) % 2 == 0);
        default: return 0;
    }
}

inline void applyMaskToData(QRGrid& qr, int pattern) {
    int n = (int)qr.matrix.size();
    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            if (!qr.isFunction[r][c]) {
                int m = maskFn(pattern, r, c);
                if (m) qr.matrix[r][c] ^= 1;
            }
        }
    }
}

// Penalty N1: runs of >=5 same color
static int penaltyN1(const QRGrid& qr) {
    int n = (int)qr.matrix.size(), score = 0;
    // Rows
    for (int r = 0; r < n; r++) {
        int runColor = qr.matrix[r][0], runLen = 1;
        for (int c = 1; c < n; c++) {
            if (qr.matrix[r][c] == runColor) runLen++;
            else {
                if (runLen >= 5) score += 3 + (runLen - 5);
                runColor = qr.matrix[r][c];
                runLen = 1;
            }
        }
        if (runLen >= 5) score += 3 + (runLen - 5);
    }
    // Columns
    for (int c = 0; c < n; c++) {
        int runColor = qr.matrix[0][c], runLen = 1;
        for (int r = 1; r < n; r++) {
            if (qr.matrix[r][c] == runColor) runLen++;
            else {
                if (runLen >= 5) score += 3 + (runLen - 5);
                runColor = qr.matrix[r][c];
                runLen = 1;
            }
        }
        if (runLen >= 5) score += 3 + (runLen - 5);
    }
    return score;
}

// Penalty N2: 2x2 blocks of same color
static int penaltyN2(const QRGrid& qr) {
    int n = (int)qr.matrix.size(), score = 0;
    for (int r = 0; r < n - 1; r++) {
        for (int c = 0; c < n - 1; c++) {
            int s = qr.matrix[r][c] + qr.matrix[r+1][c] + qr.matrix[r][c+1] + qr.matrix[r+1][c+1];
            if (s == 0 || s == 4) score += 3;
        }
    }
    return score;
}

// Penalty N3: patterns like 1:1:3:1:1 with surrounding light/dark
static int penaltyN3(const QRGrid& qr) {
    int n = (int)qr.matrix.size(), score = 0;
    const int A[11] = {1,0,1,1,1,0,1,0,0,0,0};
    const int B[11] = {0,0,0,0,1,0,1,1,1,0,1};

    // Rows
    for (int r = 0; r < n; r++) {
        for (int c = 0; c + 10 < n; c++) {
            bool okA = true, okB = true;
            for (int k = 0; k < 11; k++) {
                int v = qr.matrix[r][c + k];
                if (v != A[k]) okA = false;
                if (v != B[k]) okB = false;
            }
            if (okA || okB) score += 40;
        }
    }
    // Columns
    for (int c = 0; c < n; c++) {
        for (int r = 0; r + 10 < n; r++) {
            bool okA = true, okB = true;
            for (int k = 0; k < 11; k++) {
                int v = qr.matrix[r + k][c];
                if (v != A[k]) okA = false;
                if (v != B[k]) okB = false;
            }
            if (okA || okB) score += 40;
        }
    }
    return score;
}

// Penalty N4: balance of dark modules
static int penaltyN4(const QRGrid& qr) {
    int n = (int)qr.matrix.size();
    int total = n * n;
    int dark = 0;
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            if (qr.matrix[r][c] == 1) dark++;
    int percent = (dark * 100) / total;
    int k = abs(percent - 50) / 5;
    return k * 10;
}

inline int maskPenalty(const QRGrid& qr) {
    return penaltyN1(qr) + penaltyN2(qr) + penaltyN3(qr) + penaltyN4(qr);
}

/*==================================================
  FORMAT AND VERSION INFORMATION (BCH)
==================================================*/

// BCH(15,5) for format: poly x^10 + x^8 + x^5 + x^4 + x^2 + x + 1 (0x537)
inline uint16_t bchFormat(uint16_t formatInfo5bits) {
    uint16_t data = formatInfo5bits << 10;
    uint16_t poly = 0x537;
    for (int i = 14; i >= 10; i--) {
        if (data & (1u << i)) data ^= (poly << (i - 10));
    }
    return (formatInfo5bits << 10) | (data & 0x03FF);
}

// Mask the format bits with 0x5412 per spec
inline uint16_t formatMaskXor(uint16_t fmt15) {
    return fmt15 ^ 0x5412;
}

// Write 15 masked format bits (bit i = (fmt>>i)&1, i=0 LSB) per ISO/IEC 18004 placement,
// matching common encoders (e.g. qrcode setup_type_info). n-6 separates finder/timing regions on row 8.
inline void writeFormatInformation(QRGrid& qr, uint16_t bits15) {
    int n = (int)qr.matrix.size();
    const int x = n - 6;

    // Vertical copy: column 8
    for (int i = 0; i < 15; ++i) {
        int b = (bits15 >> i) & 1;
        int r, c = 8;
        if (i < 6)
            r = i;
        else if (i < 8)
            r = i + 1;
        else
            r = n - 15 + i;
        qr.matrix[r][c] = b;
    }
    // Horizontal copy: row 8
    for (int i = 0; i < 15; ++i) {
        int b = (bits15 >> i) & 1;
        int row = 8, c;
        if (i < 8)
            c = n - i - 1;
        else if (i < 9)
            c = x - i;
        else
            c = x - i - 1;
        qr.matrix[row][c] = b;
    }
    // Always-dark module (timing/format adjacent)
    qr.matrix[n - 8][8] = 1;
}


// BCH(18,6) for version information: poly 0x1F25
inline uint32_t bchVersion(uint32_t version) {
    uint32_t data = version << 12;
    uint32_t poly = 0x1F25;
    for (int i = 17; i >= 12; i--) {
        if (data & (1u << i)) data ^= (poly << (i - 12));
    }
    return (version << 12) | (data & 0x0FFF);
}

// Write 18 version bits (MSB-first) into reserved cells (version >= 7)
inline void writeVersionInformation(QRGrid& qr, uint32_t versionBits18) {
    int n = (int)qr.matrix.size();
    // Top-right: rows 0..5, cols n-11..n-9
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 3; c++) {
            int bit = (versionBits18 >> (17 - (r * 3 + c))) & 1;
            qr.matrix[r][n - 11 + c] = bit;
        }
    }
    // Bottom-left: rows n-11..n-9, cols 0..5
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 3; c++) {
            int bit = (versionBits18 >> (17 - (r * 3 + c))) & 1;
            qr.matrix[n - 11 + c][r] = bit;
        }
    }
}

/*==================================================
  MASK SELECTION AND RENDERING
==================================================*/

inline void selectAndApplyBestMask(QRGrid& qr, int& chosenMask) {
    int bestScore = numeric_limits<int>::max();
    int bestMask = 0;
    QRGrid best = qr;

    for (int m = 0; m < 8; m++) {
        QRGrid tmp = qr; // copy before masking
        applyMaskToData(tmp, m);
        int score = maskPenalty(tmp);
        if (score < bestScore) {
            bestScore = score;
            bestMask = m;
            best = std::move(tmp);
        }
    }
    chosenMask = bestMask;
    qr = std::move(best);
}

inline void printQR(const QRGrid& qr) {
    for (const auto& row : qr.matrix) {
        for (int v : row) std::cout << (v == 1 ? "O" : "  ");
        std::cout << '\n';
    }
}

/*==================================================
  END-TO-END MATRIX BUILD (codewords provided)
==================================================*/

// eccLevel: 'L','M','Q','H'
inline void buildQRMatrix(
    const vector<unsigned char>& interleavedCodewords,
    int version,
    char eccLevel,
    QRGrid& outGrid
) {
    // 1) Initialize function modules
    QRGrid qr = initializeQRMatrix(version);

    // 2) Place data bits
    placeDataBits(qr, interleavedCodewords);

    // 3) Select and apply best mask
    int chosenMask = 0;
    selectAndApplyBestMask(qr, chosenMask);

    // 4) Compute format bits: ECC (2 bits) + mask (3 bits), then BCH(15,5) and XOR with 0x5412
    uint16_t ecc2 =
        (eccLevel == 'L' ? 0b01 :
         eccLevel == 'M' ? 0b00 :
         eccLevel == 'Q' ? 0b11 :
                           0b10);
    uint16_t format5 = (ecc2 << 3) | (chosenMask & 0x7);
    uint16_t fmt15 = bchFormat(format5);
    uint16_t fmtMasked15 = formatMaskXor(fmt15);
    writeFormatInformation(qr, fmtMasked15);

    // 5) Version info (>=7)
    if (version >= 7) {
        uint32_t vBits = bchVersion((uint32_t)version);
        writeVersionInformation(qr, vBits);
    }

    // 6) Output
    outGrid = std::move(qr);
}
// --- END MatrixUtils.h ---



    class QRCode {
    public:
        int size;
        vector<vector<int>> matrix;

        QRCode(const string& text, char ecl = 'L') {
            if (ecl != 'L' && ecl != 'M' && ecl != 'Q' && ecl != 'H') {
                ecl = 'L';
            }

            vector<ModeEncoder*> encoders = {
                new NumericEncoder(),
                new AlphanumericEncoder(),
                new ByteEncoder()
            };

            ModeEncoder* chosenEncoder = nullptr;
            for (ModeEncoder* enc : encoders) {
                if (enc->canEncode(text)) {
                    chosenEncoder = enc;
                    break;
                }
            }

            if (!chosenEncoder) {
                for (ModeEncoder* enc : encoders) delete enc;
                throw std::runtime_error("No valid encoder found for payload");
            }

            vector<bool> encodedBits = chosenEncoder->encode(text, ecl);
            int version = decideVersion(encodedBits, ecl);
            if (version == -1) {
                for (ModeEncoder* enc : encoders) delete enc;
                throw std::runtime_error("Data too large for QR Code generation");
            }

            applyTerminatorAndPadding(encodedBits, version, ecl);

            vector<unsigned char> temp = bitsToCodeWords(encodedBits);
            vector<unsigned char> result = buildInterleavedCodewords(temp, version, ecl);

            QRGrid Q;
            buildQRMatrix(result, version, ecl, Q);

            for (ModeEncoder* enc : encoders) delete enc;

            size = Q.matrix.size();
            matrix = Q.matrix;
        }

        string toSVG(int pad = 4, const string& fg = "black", const string& bg = "white") const {
            int vb = size + pad * 2;
            string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 " + to_string(vb) + " " + to_string(vb) + "\" shape-rendering=\"crispEdges\">\n";
            svg += "  <rect width=\"100%\" height=\"100%\" fill=\"" + bg + "\" />\n";
            
            string pathD = "";
            for (int r = 0; r < size; r++) {
                for (int c = 0; c < size; c++) {
                    if (matrix[r][c] == 1) {
                        pathD += "M" + to_string(c) + "," + to_string(r) + "v1h1v-1z ";
                    }
                }
            }
            svg += "  <path d=\"" + pathD + "\" fill=\"" + fg + "\" transform=\"translate(" + to_string(pad) + ", " + to_string(pad) + ")\" />\n";
            svg += "</svg>";
            return svg;
        }

        bool saveToSVGFile(const string& filepath, int pad = 4, const string& fg = "black", const string& bg = "white") const {
            #include <fstream>
            std::ofstream out(filepath);
            if (!out) return false;
            out << toSVG(pad, fg, bg);
            out.close();
            return true;
        }

#ifdef QRCRAFT_ENABLE_PNG
        // To use this function, you must include "stb_image_write.h" in your project
        // BEFORE including "qrcraft.hpp", and define QRCRAFT_ENABLE_PNG
        void saveToPNG(const std::string& filename, int scale = 10, int border = 4) const {
            int imgSize = (size + 2 * border) * scale;
            std::vector<unsigned char> img(imgSize * imgSize * 3, 255); // RGB white background
            for (int r = 0; r < size; r++) {
                for (int c = 0; c < size; c++) {
                    if (matrix[r][c] == 1) {
                        int y0 = (r + border) * scale;
                        int x0 = (c + border) * scale;
                        for (int dy = 0; dy < scale; dy++) {
                            for (int dx = 0; dx < scale; dx++) {
                                int idx = (y0 + dy) * imgSize * 3 + (x0 + dx) * 3;
                                img[idx] = 0; img[idx+1] = 0; img[idx+2] = 0; // Black
                            }
                        }
                    }
                }
            }
            stbi_write_png(filename.c_str(), imgSize, imgSize, 3, img.data(), imgSize * 3);
        }
#endif
        
        void printTerminal() const {
            for (int r = 0; r < size; r++) {
                for (int c = 0; c < size; c++) {
                    cout << (matrix[r][c] == 1 ? "██" : "  ");
                }
                cout << "\n";
            }
        }
    };
} // namespace qrcraft
