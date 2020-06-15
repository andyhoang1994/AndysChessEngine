#include "bitboard.h"
#include <iostream>

unsigned int firstOne(Bitboard bitmap)
{
    // De Bruijn Multiplication, see http://chessprogramming.wikispaces.com/BitScan
    // don't use this if bitmap = 0

    static const int INDEX64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5 };

    static const Bitboard DEBRUIJN64 = Bitboard(0x07EDD5E59A4E28C2);

    // here you would get a warming: "unary minus operator applied to unsigned type",
    // that's intended and OK so I'll disable it
    #pragma warning (disable: 4146)
    return INDEX64[((bitmap & -bitmap) * DEBRUIJN64) >> 58];
}

unsigned int lastOne(Bitboard bitmap) {
    // this is Eugene Nalimov's bitScanReverse
    // use firstOne if you can, it is faster than lastOne.
    // don't use this if bitmap = 0

    int MS1BTABLE[256];
    for(int i = 0; i < 256; i++) {
        MS1BTABLE[i] = (
            (i > 127) ? 7 :
            (i > 63) ? 6 :
            (i > 31) ? 5 :
            (i > 15) ? 4 :
            (i > 7) ? 3 :
            (i > 3) ? 2 :
            (i > 1) ? 1 : 0
        );
    }
    int result = 0;
    if(bitmap > 0xFFFFFFFF)
    {
        bitmap >>= 32;
        result = 32;
    }
    if(bitmap > 0xFFFF)
    {
        bitmap >>= 16;
        result += 16;
    }
    if(bitmap > 0xFF)
    {
        bitmap >>= 8;
        result += 8;
    }
    return result + MS1BTABLE[bitmap];
}