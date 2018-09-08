﻿#ifndef C_SHA3_256_H
#define C_SHA3_256_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void cryptopop_sha3_256(uint8_t *input, uint32_t inputLen, uint8_t *output);

#ifdef __cplusplus
}
#endif


#endif
