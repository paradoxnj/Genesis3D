#ifndef BIT_H
#define BIT_H

//===========================
// Bit manipulation
//===========================

#define TEST(flags,mask) ((flags) & (mask))
#define SET(flags,mask) ((flags) |= (mask))
#define RESET(flags,mask) ((flags) &= ~(mask))
#define FLIP(flags,mask) ((flags) ^= (mask))

// mask definitions
#define BIT(shift)     (1<<(shift))

#define SET_BITARR(bitarr, bitnum) (bitarr[(bitnum)>>3] |= (1<<((bitnum)&7)))
#define RESET_BITARR(bitarr, bitnum) (bitarr[(bitnum)>>3] &= ~(1<<((bitnum)&7)))
#define TEST_BITARR(bitarr, bitnum) (bitarr[(bitnum)>>3] & (1<<((bitnum)&7)))
#endif