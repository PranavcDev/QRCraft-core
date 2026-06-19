#pragma once
#include <vector>
#include <cstdint>
#include <cassert>

#include "VersionUtils.hpp"
#include "ErrorCorrection.hpp"

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
            QRCode::ErrorCorrection::computeECCCodewords(dat, blockEccLen);
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
