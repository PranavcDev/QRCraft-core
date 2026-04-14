#pragma once
#include <vector>
using namespace std;

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
}