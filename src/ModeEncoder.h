#pragma once
#include <iostream>
#include <vector>
#include <string>
using namespace std;

class ModeEncoder{  // interface for all encoding modes
public:
    virtual bool canEncode(const string& data) = 0;
    virtual vector<bool> encode(const string& data, char ecl) = 0;
    virtual int getCharacterCountBits(int version) = 0;
    virtual ~ModeEncoder() {}
};