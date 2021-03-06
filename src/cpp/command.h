#pragma once

#include "base.h"
#include "coordinate_difference.h"

struct Command
{
    enum Type
    {
        Halt, Wait, Flip, SMove, LMove, FusionP, FusionS, Fission, Fill, Void, GFill, GVoid
    };

    Type type;
    CoordinateDifference cd1, cd2;
    unsigned m;

    Command(Type _type = Halt) : type(_type) {}

    void Encode(vector<uint8_t>& v) const;
    void Decode(const vector<uint8_t>& v, size_t& pos);

    size_t Energy() const;

protected:
    static uint16_t EncodeSCD(const CoordinateDifference& cd)
    {
        assert(cd.IsShortLinearCoordinateDifferences());
        if (cd.dx)
            return (1 << 8) + (cd.dx + 5);
        else if (cd.dy)
            return (2 << 8) + (cd.dy + 5);
        else
            return (3 << 8) + (cd.dz + 5);
    }

    static uint16_t EncodeLCD(const CoordinateDifference& cd)
    {
        assert(cd.IsLongLinearCoordinateDifferences());
        if (cd.dx)
            return (1 << 8) + (cd.dx + 15);
        else if (cd.dy)
            return (2 << 8) + (cd.dy + 15);
        else
            return (3 << 8) + (cd.dz + 15);
    }

    static uint8_t EncodeNCD(const CoordinateDifference& cd)
    {
        assert(cd.IsNearCoordinateDifferences());
        return 9 * (cd.dx + 1) + 3 * (cd.dy + 1) + (cd.dz + 1);
    }

    static uint32_t EncodeFCD(const CoordinateDifference& cd)
    {
        assert(cd.IsFarCoordinateDifferences());
        int32_t shift = 30;
        return (((cd.dx + shift) << 16) | ((cd.dy + shift) << 8) | (cd.dz + shift));
    }

    static CoordinateDifference DecodeSCD(uint16_t t)
    {
        uint16_t a = (t >> 8);
        int d = int(t & 255) - 5;
        if (a == 1)
            return CoordinateDifference{d, 0, 0};
        else if (a == 2)
            return CoordinateDifference{0, d, 0};
        else if (a == 3)
            return CoordinateDifference{0, 0, d};
        assert(false);
        return CoordinateDifference();
    }

    static CoordinateDifference DecodeLCD(uint16_t t)
    {
        uint16_t a = (t >> 8);
        int d = int(t & 255) - 15;
        if (a == 1)
            return CoordinateDifference{d, 0, 0};
        else if (a == 2)
            return CoordinateDifference{0, d, 0};
        else if (a == 3)
            return CoordinateDifference{0, 0, d};
        assert(false);
        return CoordinateDifference();
    }

    static CoordinateDifference DecodeNCD(uint8_t t)
    {
        int d = int(t);
        int dz = (d % 3) - 1; d /= 3;
        int dy = (d % 3) - 1; d /= 3;
        int dx = (d % 3) - 1;
        return CoordinateDifference{dx, dy, dz};
    }

    static CoordinateDifference DecodeFCD(uint32_t t)
    {
        int dz = int(t & 255) - 30; t >>= 8;
        int dy = int(t & 255) - 30; t >>= 8;
        int dx = int(t & 255) - 30;
        return CoordinateDifference{dx, dy, dz};
    }
};

ostream& operator<<(ostream& s, const Command& c);
