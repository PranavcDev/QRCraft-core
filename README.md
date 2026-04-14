# QRCraft

A incredibly powerful, high-performance C++ QR Code Engine.

QRCraft mathematically processes input strings through Alphanumeric, Numeric, and Byte encodings seamlessly, computes Reed-Solomon Error Correction, and generates a native, iterable 2D module array representing the complete QR Code—all natively without strictly requiring heavy external dependencies.

This repository provides **two** distinct ways to execute the engine depending on your preference:
1. **The Single-Header Library:** A unified, dependency-free `.hpp` file intended for lightning-fast drop-in usage in generic C++ projects.
2. **The Modular Source Code:** The original, fragmented `.h` and `.cpp` structure equipped with a CMake build system (ideal for advanced contributing and deep logic modifications).

---

## Option A: The Single-Header Library (Recommended)
Because QRCraft features an STB-style single-header distribution, there are no complex makefiles or object links required for generic users.  

Just drop the `include/qrcraft.hpp` file directly into your C++ folder!

### 1. The C++ Code (`main.cpp`)
```cpp
#include <iostream>
// 1. Include the single header library into your code
#include "qrcraft.hpp"

// Optional: Want to generate PNG files instead of SVGs? Simply define these two lines BEFORE including qrcraft.hpp!
// #define QRCRAFT_ENABLE_PNG
// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include "stb_image_write.h"

int main() {
    // 2. Initialize the QRCode with your Link and Error Correction Level ('L', 'M', 'Q', 'H')
    qrcraft::QRCode qr("https://mywebsite.com", 'H');
    
    std::cout << "Successfully generated a " << qr.size << "x" << qr.size << " QR Matrix!" << std::endl;

    // 3. (Optional) Save it losslessly to your drive as an SVG!
    qr.saveToSVGFile("my_code.svg");

    // 4. Need to instantly view it natively in the terminal?
    qr.printTerminal();
    
    return 0;
}
```

### 2. How to compile
Because everything is natively inlined, you just compile your source file directly using `g++` or `clang++`.
```bash
g++ main.cpp -o qr generator -std=c++17 -I ./include
./qr_generator
```

---

## Option B: Executing the Modular Codebase (Advanced/CMake)
If you want to edit the internal mathematics, encoders, or error correction implementations natively, the `src/` directory contains the fully modular architecture.

### 1. The C++ Code (`src/main.cpp`)
The modular architecture exposes the underlying implementation algorithms to you. You can review `src/main.cpp` to witness the raw pipeline of encoding, padding, and block interleaving.
It natively uses `#include "Renderer.h"` and relies on `stb_image_write.h` to output a bitmap `.png` file representing the generated QR structure.

### 2. How to compile (CMake)
The modular code is orchestrated utilizing CMake. Ensure you have `cmake` and `make` (or Visual Studio) installed.

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Generate the Makefiles
cmake ..

# 3. Compile the executable
cmake --build .

# 4. Run the generator!
./qr_generator
```

---
## Features Included
* Auto-Optimization of `Numeric`, `Alphanumeric`, and `Byte` modes for data storage density.
* Direct memory integration: Immediately returns clean `std::vector<std::vector<int>>` matrix arrays to manipulate in custom graphical engines (OpenGL, iOS Canvas, WebAssembly, etc).
* Isolated header namespace (`qrcraft::`) strictly preventing global ODR pollution.

## License
This project is open-sourced under the generous `MIT License`. You are free to modify, distribute, and monetize any integration utilizing this engine natively or commercially!
