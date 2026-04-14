#include <emscripten/bind.h>
#include <vector>
#include <string>
#include <stdexcept>
#include "qrcraft.hpp"

using namespace emscripten;

// This function acts as the bridge.
// Returns a 1D vector: [dimension, matrix data...]
std::vector<int> generate_qr_wasm(std::string input, char ecl) {
    try {
        // Utilize the elegantly unified single-header API
        qrcraft::QRCode Q(input, ecl);
        
        int dimension = Q.size;
        std::vector<int> flat;
        flat.reserve(1 + dimension * dimension);
        
        // Position 0 stores the dimension
        flat.push_back(dimension);

        for (int r = 0; r < dimension; r++) {
            for (int c = 0; c < dimension; c++) {
                flat.push_back(Q.matrix[r][c] == 1 ? 1 : 0);
            }
        }
        return flat;
        
    } catch (const std::exception& e) {
        // Catch constraints (e.g. payload too large) 
        // to gracefully degrade in JavaScript
        return {-1}; 
    }
}

EMSCRIPTEN_BINDINGS(qr_module) {
    function("generate_qr_wasm", &generate_qr_wasm);
    register_vector<int>("VectorInt");
}
