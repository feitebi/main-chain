﻿#include "c_sha1.h"

#include <stdint.h>
#include <string.h>
#include <openssl/sha.h>

#include "common.h"

void cryptopop_sha1(uint8_t *input, uint32_t inputLen, uint8_t *output) {
	uint32_t i;
	uint8_t result[(SHA_DIGEST_LENGTH) << 1];

	SHA_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, input, inputLen);
	SHA1_Final(result, &ctx);

	uint8_t inputStr[INPUT_LEN];
	for(i = 0; i < inputLen; ++i)
		inputStr[i] = ~(input[i]);
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, inputStr, inputLen);
	SHA1_Final(result + SHA_DIGEST_LENGTH, &ctx);

	reduce_bit(result, (SHA_DIGEST_LENGTH) << 1, output, 256);
}
