#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "MatrixUtils.h"

void renderQRToPNG(
    const QRGrid& qr,
    const std::string& filename,
    int scale = 10,      // pixels per module
    int border = 4       // quiet zone (modules)
) {
    int n = qr.matrix.size();
    int imgSize = (n + 2 * border) * scale;


    std::vector<unsigned char> img(imgSize * imgSize * 3, 255); // RGB


    for (int r = 0; r < n; r++) {
        for (int c = 0; c < n; c++) {
            if (qr.matrix[r][c] == 1) {
                int y0 = (r + border) * scale;
                int x0 = (c + border) * scale;
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {

                        int idx = (y0 + dy) * imgSize * 3 + (x0 + dx) * 3;
                        img[idx] = 0; img[idx+1] = 0; img[idx+2] = 0; // RGB black

                    }
                }
            }
        }
    }


    stbi_write_png(
        filename.c_str(),
        imgSize,
        imgSize,
        3,                  // 3 channel RGB
        img.data(),
        imgSize * 3
    );

}
