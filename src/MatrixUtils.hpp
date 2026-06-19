#pragma once
#include <vector>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <limits>

using namespace std;

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
        for (int v : row) std::cout << (v == 1 ? "██" : "  ");
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
