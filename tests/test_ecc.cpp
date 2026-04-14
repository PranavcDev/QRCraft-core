#include "../src/ErrorCorrection.h"
#include <iostream>
#include <cassert>

int main() {
    using namespace QRCode::ErrorCorrection;

    // Russian-peasant multiply in GF(256) with reduction 0x1D
    assert(multiplyGF(2, 87) == 174);
    std::cout << "multiplyGF OK" << std::endl;

    // Test generator poly for n=3: should be [1, 2, 4, 8] reversed or equiv
    auto g3 = generateGeneratorPoly(3);
    assert(g3.size() == 3 && g3[0] == 7 && g3[1] == 14 && g3[2] == 8);
    std::cout << "generateGeneratorPoly OK" << std::endl;

    auto data = std::vector<unsigned char>{85};
    auto ecc = computeECCCodewords(data, 2);
    assert(ecc.size() == 2 && ecc[0] == 255 && ecc[1] == 170);
    std::cout << "computeECCCodewords OK: [" << (int)ecc[0] << ", " << (int)ecc[1] << "]" << std::endl;

    std::cout << "All ErrorCorrection tests passed!" << std::endl;
    return 0;
}

