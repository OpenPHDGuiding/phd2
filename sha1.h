/*********************************************************************
* Filename:   sha1.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA1_H
#define SHA1_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA1_BLOCK_SIZE 20              // SHA1 outputs a 20 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char SHA1_BYTE;             // 8-bit byte
typedef unsigned int  SHA1_WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	SHA1_BYTE data[64];
	SHA1_WORD datalen;
	unsigned long long bitlen;
	SHA1_WORD state[5];
	SHA1_WORD k[4];
} SHA1_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha1_init(SHA1_CTX *ctx);
void sha1_update(SHA1_CTX *ctx, const SHA1_BYTE data[], size_t len);
void sha1_final(SHA1_CTX *ctx, SHA1_BYTE hash[]);

#endif   // SHA1_H
