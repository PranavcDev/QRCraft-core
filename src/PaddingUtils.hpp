#pragma once
#include "VersionUtils.hpp"
#include <vector>

inline void applyTerminatorAndPadding(std::vector<bool>& bits, int version, char ecl){
    int capacity = getMaxDataBits(version, ecl);

    // terminator
    int remaining = capacity - bits.size();
    int terminator = (remaining >= 4) ? 4 : remaining;
    for(int i = 0; i < terminator; i++)
        bits.push_back(0);

    // alignment of byte
    while(bits.size() % 8 != 0)
        bits.push_back(0);

    // pad bytes
    bool toggle = true;
    while(bits.size() < capacity){
        unsigned char pad = toggle ? 0xEC : 0x11;
        toggle = !toggle;

        for(int i = 7; i >= 0; i--)
            bits.push_back((pad >> i) & 1);
    }
}