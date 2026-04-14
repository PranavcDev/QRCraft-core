#include "ModeEncoder.h"
#include "VersionUtils.h"
#include <unordered_map>

using namespace std;

class AlphanumericEncoder : public ModeEncoder {
    unordered_map<char, int> table = {
        {'0',0},{'1',1},{'2',2},{'3',3},{'4',4},{'5',5},{'6',6},{'7',7},{'8',8},{'9',9},
        {'A',10},{'B',11},{'C',12},{'D',13},{'E',14},{'F',15},{'G',16},{'H',17},{'I',18},{'J',19},
        {'K',20},{'L',21},{'M',22},{'N',23},{'O',24},{'P',25},{'Q',26},{'R',27},{'S',28},{'T',29},
        {'U',30},{'V',31},{'W',32},{'X',33},{'Y',34},{'Z',35},{' ',36},{'$',37},{'%',38},{'*',39},
        {'+',40},{'-',41},{'.',42},{'/',43},{':',44}
    };
public:
    bool canEncode(const string &data){
        for (char c : data)
            if (table.find(c) == table.end()) return false;
        return true;
    }

    vector<bool> encode(const string& data, char ecl) {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator: 0010
        bits.push_back(0); bits.push_back(0); bits.push_back(1); bits.push_back(0);

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

        // encoding all the input into the bits with respect to the encoding rule for the alphanumeric string
        // for the group of two : 11 bits required and the value is appended
        // for only one char : 6 bits required and the value is appended
        // value (for group of two) = value of first char from map * 45 + value of second char from the map
        // value (for single char) = corresponding value of the char from the map
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 2) {
                int value = table[data[i]] * 45 + table[data[i + 1]];
                for (int j = 10; j >= 0; j--) bits.push_back((value >> j) & 1); // 11 bits
                i += 2;
            }
            else { // remaining == 1
                int value = table[data[i]];
                for (int j = 5; j >= 0; j--) bits.push_back((value >> j) & 1); // 6 bits
                i += 1;
            }
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int len = data.size();
        int i = 0;
        while (i < len) {
            int remaining = len - i;

            if (remaining >= 2) {
                int value = table[data[i]] * 45 + table[data[i + 1]];
                for (int j = 10; j >= 0; j--) bits.push_back((value >> j) & 1); // 11 bits
                i += 2;
            }
            else { // remaining == 1
                int value = table[data[i]];
                for (int j = 5; j >= 0; j--) bits.push_back((value >> j) & 1); // 6 bits
                i += 1;
            }
        }

        return bits;
    }

    int getCharacterCountBits(int version){
        if (version >= 1 && version <= 9) return 9;
        else if (version <= 26) return 11;
        else return 13;
    }
};