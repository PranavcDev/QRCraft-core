#include <string>
#include "ModeEncoder.h"
#include "VersionUtils.h"
#include <cctype>
using namespace std;

class NumericEncoder : public ModeEncoder{
public:
    bool canEncode(const string& data){
        for(char c : data)
            if(!isdigit(c)) return false;
        return true; 
    }

    vector<bool> encode(const string& data, char ecl) {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator: 0001
        bits.push_back(0); bits.push_back(0); bits.push_back(0); bits.push_back(1);

        int version = 1;
        for (; version <= 40; ++version) {
            int total = 4 + getCharacterCountBits(version) + (int)rawBits.size();
            if (total <= getMaxDataBits(version, ecl))
                break;
        }
        int characterBits = getCharacterCountBits(version);

        // Character count bits
        int len = data.size();
        for (int i = characterBits - 1; i >= 0; i--)
            bits.push_back((len >> i) & 1);

        // encoding the the data string into the bits 
        // for the group of 3 chars : required bits = 10
        // for the group of 2 chars : required bits = 7
        // for the group of single char : required bits = 4
        // all the values of groups of 3 chars, 2 chars and single char are corresponding integer value
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 3) {
                int value = (data[i] - '0') * 100 + (data[i + 1] - '0') * 10 + (data[i + 2] - '0');
                for (int j = 9; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 3;
            }
            else if (remaining == 2) {
                int value = (data[i] - '0') * 10 + (data[i + 1] - '0');
                for (int j = 6; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 2;
            }
            else { // remaining == 1
                int value = data[i] - '0';
                for (int j = 3; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 1;
            }
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int i = 0, len = data.size();
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 3) {
                int value = (data[i] - '0') * 100 + (data[i + 1] - '0') * 10 + (data[i + 2] - '0');
                for (int j = 9; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 3;
            }
            else if (remaining == 2) {
                int value = (data[i] - '0') * 10 + (data[i + 1] - '0');
                for (int j = 6; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 2;
            }
            else { // remaining == 1
                int value = data[i] - '0';
                for (int j = 3; j >= 0; j--) bits.push_back((value >> j) & 1);
                i += 1;
            }
        }

        return bits;
    }

    int getCharacterCountBits(int version){
        if(version >= 1 && version <= 9) return 10;
        else if(version <= 26) return 12;
        else return 14;
    }
};