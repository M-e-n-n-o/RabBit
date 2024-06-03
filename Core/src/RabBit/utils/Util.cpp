#include "RabBitCommon.h"
#include "Util.h"

namespace RB
{
    // https://stackoverflow.com/questions/109023/count-the-number-of-set-bits-in-a-32-bit-integer
    uint32_t NumberOfSetBits(uint32_t bitset)
    {
        bitset = bitset - ((bitset >> 1) & 0x55555555);                 // add pairs of bits
        bitset = (bitset & 0x33333333) + ((bitset >> 2) & 0x33333333);  // quads
        bitset = (bitset + (bitset >> 4)) & 0x0F0F0F0F;                 // groups of 8
        bitset *= 0x01010101;                                           // horizontal sum of bytes
        return  bitset >> 24;                                           // return just that top byte (after truncating to 32-bit even when int is wider than uint32_t)
    }
}