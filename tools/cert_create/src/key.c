/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include "cert.h"
#include "debug.h"
#include "key.h"
#include "platform_oid.h"
#include "sha.h"

#define MAX_FILENAME_LEN		1024

/*
 * Create a new key container
 */
static int key_new(key_t *key)
{
	/* Create key pair container */
	key->key = EVP_PKEY_new();
	if (key->key == NULL) {
		return 0;
	}

	return 1;
}

int key_create(key_t *key, int type)
{
	RSA *rsa = NULL;
	EC_KEY *ec = NULL;

	/* Create OpenSSL key container */
	if (!key_new(key)) {
		goto err;
	}

	switch (type) {
	case KEY_ALG_RSA:
		/* Generate a new RSA key */
		rsa = RSA_generate_key(RSA_KEY_BITS, RSA_F4, NULL, NULL);
		if (rsa == NULL) {
			printf("Cannot create RSA key\n");
			goto err;
		}
		if (!EVP_PKEY_assign_RSA(key->key, rsa)) {
			printf("Cannot assign RSA key\n");
			goto err;
		}
		break;
	case KEY_ALG_ECDSA:
		/* Generate a new ECDSA key */
		ec = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
		if (ec == NULL) {
			printf("Cannot create EC key\n");
			goto err;
		}
		if (!EC_KEY_generate_key(ec)) {
			printf("Cannot generate EC key\n");
			goto err;
		}
		EC_KEY_set_flags(ec, EC_PKEY_NO_PARAMETERS);
		EC_KEY_set_asn1_flag(ec, OPENSSL_EC_NAMED_CURVE);
		if (!EVP_PKEY_assign_EC_KEY(key->key, ec)) {
			printf("Cannot assign EC key\n");
			goto err;
		}
		break;
	default:
		goto err;
	}

	return 1;

err:
	RSA_free(rsa);
	EC_KEY_free(ec);

	return 0;
}

int key_load(key_t *key, unsigned int *err_code)
{
	FILE *fp = NULL;
	EVP_PKEY *k = NULL;

	/* Create OpenSSL key container */
	if (!key_new(key)) {
		*err_code = KEY_ERR_MALLOC;
		return 0;
	}

	if (key->fn) {
		/* Load key from file */
		fp = fopen(key->fn, "r");
		if (fp) {
			k = PEM_read_PrivateKey(fp, &key->key, NULL, NULL);
			fclose(fp);
			if (k) {
				*err_code = KEY_ERR_NONE;
				return 1;
			} else {
				ERROR("Cannot load key from %s\n", key->fn);
				*err_code = KEY_ERR_LOAD;
			}
		} else {
			WARN("Cannot open file %s\n", key->fn);
			*err_code = KEY_ERR_OPEN;
		}
	} else {
		WARN("Key filename not specified\n");
		*err_code = KEY_ERR_FILENAME;
	}

	return 0;
}

int key_store(key_t *key)
{
	FILE *fp = NULL;

	if (key->fn) {
		fp = fopen(key->fn, "w");
		if (fp) {
			PEM_write_PrivateKey(fp, key->key,
					NULL, NULL, 0, NULL, NULL);
			fclose(fp);
			return 1;
		} else {
			ERROR("Cannot create file %s\n", key->fn);
		}
	} else {
		ERROR("Key filename not specified\n");
	}

	return 0;
}
