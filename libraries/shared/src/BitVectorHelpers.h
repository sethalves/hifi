//
//  BitVectorHelpers.h
//  libraries/shared/src
//
//  Created by Anthony Thibault on 1/19/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BitVectorHelpers_h
#define hifi_BitVectorHelpers_h

#include <stdint.h>
#include <assert.h>

#include "NumericalConstants.h"

//
// BitVector compression
//
const int MAX_BITVECTOR_BITS = 256;
const int MAX_BITVECTOR_BYTES = (MAX_BITVECTOR_BITS + 7) / 8;

// For this coding scheme, the worst case is 2 x uncompressed size,
// and bitstream reader/writer may overread/overwrite by 1 byte.
const int MAX_CODE_BYTES = 2 * MAX_BITVECTOR_BYTES + 1;

// Encode bitVector[numBits] into codeBytes[MAX_CODE_BYTES]
// returns the number of actual bytes used
int encodeBitVector(bool* bitVector, int numBits, uint8_t* codeBytes, int maxCodeBytes);

// Decode codeBytes[maxCodeBytes] into bitVector[numBits]
// NOTE: numBits must already be known at the decoder
// return the number of actual bytes used
int decodeBitVector(const uint8_t* codeBytes, int maxCodeBytes, bool* bitVector, int numBits);


static inline int calcBitVectorSize(int numBits) {
    return (numBits + 7) / 8;
}

static inline int calcMaxCompressedBitVectorSize(int numBits) {
    int bitVectorSize = calcBitVectorSize(numBits);
    return 2 * bitVectorSize + 1;
}

// func should be of type bool func(int index)
template <typename F>
int writeBitVector(uint8_t* destinationBuffer, int numBits, const F& func) {

    bool bitVector[MAX_BITVECTOR_BITS];
    for (int i = 0; i < numBits; i++) {
        bitVector[i] = func(i);
    }

    return encodeBitVector(bitVector, numBits, destinationBuffer, MAX_CODE_BYTES);
}

// func should be of type 'void func(int index, bool value)'
template <typename F>
int readBitVector(const uint8_t* sourceBuffer, int numBits, const F& func) {

    bool bitVector[MAX_BITVECTOR_BITS];
    int totalBytes = decodeBitVector(sourceBuffer, MAX_CODE_BYTES, bitVector, numBits);

    for (int i = 0; i < numBits; i++) {
        func(i, bitVector[i]);
    }
    return totalBytes;
}

#endif
