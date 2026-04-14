import os
import re

HEADER_FILES = [
    "VersionUtils.h",
    "ModeEncoder.h",
    "NumericEncoder.h",
    "AlphanumericEncoder.h",
    "ByteEncoder.h",
    "PaddingUtils.h",
    "CodewordUtils.h",
    "ErrorCorrection.h",
    "VersionBlocks.h",
    "DataInterleaving.h",
    "MatrixUtils.h"
]

SRC_DIR = "src"
OUT_FILE = "include/qrcraft.hpp"

std_includes = set()
content_blocks = []

def process_file(filename):
    path = os.path.join(SRC_DIR, filename)
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
        
    block = f"// --- BEGIN {filename} ---\n"
    for line in lines:
        stripped = line.strip()
        if stripped == "#pragma once":
            continue
        if stripped.startswith("#include <") and stripped.endswith(">"):
            std_includes.add(stripped)
            continue
        if stripped.startswith('#include "') and stripped.endswith('"'):
            continue
        if stripped == "using namespace std;" or stripped == "using namespace QRCode;":
            continue
            
        line = line.replace("QRCode::ErrorCorrection::", "qrmath::ErrorCorrection::")
        line = line.replace("namespace QRCode {", "namespace qrmath {")
        
        block += line
        
    block += f"// --- END {filename} ---\n\n"
    return block

for h in HEADER_FILES:
    content_blocks.append(process_file(h))

out_dir = os.path.dirname(OUT_FILE)
if not os.path.exists(out_dir):
    os.makedirs(out_dir)

std_includes.add("#include <fstream>")

with open(OUT_FILE, "w", encoding="utf-8") as f:
    f.write("#pragma once\n\n")
    
    for inc in sorted(std_includes):
        f.write(inc + "\n")
        
    f.write("\nnamespace qrcraft {\n")
    f.write("using namespace std; // Isolated to qrcraft namespace\n\n")
    
    for b in content_blocks:
        f.write(b)
        
    f.write(r'''

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
''')

print("Successfully merged into", OUT_FILE)
