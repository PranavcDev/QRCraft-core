#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "ModeEncoder.hpp"
#include "NumericEncoder.hpp"
#include "AlphanumericEncoder.hpp"
#include "ByteEncoder.hpp"
#include "VersionUtils.hpp"
#include "MatrixUtils.hpp"
#include "PaddingUtils.hpp"
#include "CodewordUtils.hpp"
#include "Renderer.hpp"
#include "VersionBlocks.hpp"

using namespace std;

int main() {
    string input = "HELLO! I'M QRCRAFT: A POWERFUL C++ QR CODE GENERATOR";
    char ecl = 'H'; // Available error correction levels: 'L', 'M', 'Q', 'H'


    // Step 2.1: Choose encoder
    vector<ModeEncoder*> encoders = {
        new NumericEncoder(),
        new AlphanumericEncoder(),
        new ByteEncoder()
    };

    ModeEncoder* chosenEncoder = nullptr;
    for (ModeEncoder* enc : encoders) {
        if (enc->canEncode(input)) {
            chosenEncoder = enc;
            break;
        }
    }

    if (!chosenEncoder) {
        cout << "No encoder can handle this input!" << endl;
        return 1;
    }

    // Step 2.2: Encode input
    vector<bool> encodedBits = chosenEncoder->encode(input, ecl);

    // Output
    if (dynamic_cast<NumericEncoder*>(chosenEncoder)) cout << "Encoder: Numeric" << endl;
    else if (dynamic_cast<AlphanumericEncoder*>(chosenEncoder)) cout << "Encoder: Alphanumeric" << endl;
    else cout << "Encoder: Byte" << endl;

    cout << "\nScanning Input: " << input << " ...\n" << endl;

    int version = decideVersion(encodedBits, ecl);

    if (version == -1) {
        cout << "Data too large for QR version 1–40" << endl;
        return 1;
    }


    applyTerminatorAndPadding(encodedBits, version, ecl);

    vector<unsigned char> temp = bitsToCodeWords(encodedBits);
    vector<unsigned char> result = buildInterleavedCodewords(temp, version, ecl);



    QRGrid Q;
    buildQRMatrix(result, version, ecl, Q);

    // Fill remaining -1 to 0 (white)
    for (auto& row : Q.matrix) {
        for (int& v : row) {
            if (v == -1) v = 0;
        }
    }

    printQR(Q);

    const std::string pngName = "qr.png";
    renderQRToPNG(Q, pngName, 10);

    std::filesystem::path outPath = std::filesystem::absolute(pngName);
    cout << "QR code saved to: " << outPath.string() << endl;
    cout << "(Scan this file; an old qr.png in another folder will not match the latest build.)" << endl;

    // Cleanup
    for (ModeEncoder* enc : encoders) delete enc;

    return 0;
}

