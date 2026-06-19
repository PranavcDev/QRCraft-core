#pragma once

#include <vector>
#include <cstdint>

namespace QRCode {
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
