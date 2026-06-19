#pragma once
#include <vector>
using namespace std;

/*
    Interleave data and ECC blocks according to QR standard.
    This ensures robustness: damage in one area affects both data & ECC in all blocks.
*/

inline vector<unsigned char> interleaveBlocks(
    const vector<vector<unsigned char>>& dataBlocks,
    const vector<vector<unsigned char>>& eccBlocks
){
    vector<unsigned char> result;

    // finding the max block length/size
    size_t maxDataLen = 0;
    for(const vector<unsigned char>& block : dataBlocks)
        if(block.size() > maxDataLen) maxDataLen = block.size();

    // interleave the data bytes first
    for(size_t i = 0; i < maxDataLen; i++){
        // appending the ith byte from each block of data
        for(const vector<unsigned char>& block : dataBlocks){
            if(i < block.size())
                result.push_back(block[i]);
        }
    }

    // interleaving ecc bytes
    size_t maxEccLen = 0;
    for(const vector<unsigned char>& block : eccBlocks)
        if(block.size() > maxEccLen) maxEccLen = block.size();

    for(size_t i = 0; i < maxEccLen; i++){
        for(const vector<unsigned char>& block : eccBlocks){
            if(i < block.size())
                result.push_back(block[i]);
        }
    }

    return result;
}