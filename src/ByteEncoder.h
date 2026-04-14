#include "ModeEncoder.h"
#include "VersionUtils.h"

using namespace std;

class ByteEncoder : public ModeEncoder{
public:
    bool canEncode(const string &data) override {
        return true;
    }

    vector<bool> encode(const string& data, char ecl) override {
        vector<bool> rawBits;
        rawBits = dataBits(data);

        vector<bool> bits;

        // Mode indicator (4 bits for byte mode)
        bits.push_back(0); bits.push_back(1); bits.push_back(0); bits.push_back(0);

        // Version for count field: smallest v where mode + count + payload fits
        int version = 1;
        for (; version <= 40; ++version) {
            int total = 4 + getCharacterCountBits(version) + (int)rawBits.size();
            if (total <= getMaxDataBits(version, ecl))
                break;
        }

        int characterBits = getCharacterCountBits(version);
        int len = data.size();
        for (int i = characterBits - 1; i >= 0; --i)
            bits.push_back((len >> i) & 1);

        // Encode each character in 8-bit ASCII
        for (unsigned char c : data) {
            for (int i = 7; i >= 0; --i)
                bits.push_back((c >> i) & 1);
        }

        return bits;
    }

    vector<bool> dataBits(const string& data){
        vector<bool> bits;

        int len = data.size();

        // Encode each character in 8-bit ASCII
        for (unsigned char c : data) {
            for (int i = 7; i >= 0; --i)
                bits.push_back((c >> i) & 1);
        }

        return bits;
    }

    int getCharacterCountBits(int version) override {
        if (version >= 1 && version <= 9) return 8;
        else return 16;
    }
};