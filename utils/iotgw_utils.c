/*
 * 	IOTGW MQTT Client
 *
 *  Created on: Sep 22, 2022
 *  Author: Richard Lee
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "json-c/json.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"
#include "iotgw_utils.h"

#define IOT_CMFT_SECKEY_LEN     20
#define IOT_CMFT_MQTT_VER_STR   "1"

#define SHA1_KEY_IOPAD_SIZE (64)
#define SHA1_DIGEST_SIZE    (20)

void utils_hmac_sha1(const char *msg, int msg_len, char *digest, const char *key, int key_len)
{
	mbedtls_sha1_context context;
	unsigned char k_ipad[SHA1_KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
	unsigned char k_opad[SHA1_KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
	unsigned char out[SHA1_DIGEST_SIZE];
	int i;

	if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
		return;
	}

	if (key_len > SHA1_KEY_IOPAD_SIZE) {
		return;
	}

	/* start out by storing key in pads */
	memset(k_ipad, 0, sizeof(k_ipad));
	memset(k_opad, 0, sizeof(k_opad));
	memcpy(k_ipad, key, key_len);
	memcpy(k_opad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i = 0; i < SHA1_KEY_IOPAD_SIZE; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/* perform inner SHA */
	mbedtls_sha1_init(&context);                                      /* init context for 1st pass */
	mbedtls_sha1_starts(&context);                                    /* setup context for 1st pass */
	mbedtls_sha1_update(&context, k_ipad, SHA1_KEY_IOPAD_SIZE);            /* start with inner pad */
	mbedtls_sha1_update(&context, (unsigned char *)msg, msg_len);    /* then text of datagram */
	mbedtls_sha1_finish(&context, out);                               /* finish up 1st pass */

	/* perform outer SHA */
	mbedtls_sha1_init(&context);                              /* init context for 2nd pass */
	mbedtls_sha1_starts(&context);                            /* setup context for 2nd pass */
	mbedtls_sha1_update(&context, k_opad, SHA1_KEY_IOPAD_SIZE);    /* start with outer pad */
	mbedtls_sha1_update(&context, out, SHA1_DIGEST_SIZE);     /* then results of 1st hash */
	mbedtls_sha1_finish(&context, out);                       /* finish up 2nd pass */
	memcpy(digest, out, SHA1_DIGEST_SIZE);
}

int utils_urlencode(const char *src, char *dst, unsigned int dstlen)
{
    int i, j;
    char c;

    if (!src || !dst) {
        return 0;
    }

    i = j = 0;
    while ((c = src[i]) != 0 && j < dstlen - 4) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z')) {
            dst[j++] = c;
        } else {
            sprintf(dst + j, "%%%02X", c);
            j += 3;
        }
        i++;
    }
    return j;
}

int iot_cmft_get_token_mqtt(const char *pid, const char *did, const char *sec, char *token, unsigned int token_max_len)
{
	char sigstr[256] = {0}, keydec[256] = {0}, digest[256] = {0}, digest_enc[256] = {0}, digest_url[256] = {0};
    size_t keylen = 0, digest_len = 0;
    long et;

    if (!pid || !did || !sec) {
        return -1;
    }

    if (mbedtls_base64_decode((unsigned char *)keydec, sizeof(keydec), &keylen, (const unsigned char *)sec, strlen(sec)) != 0) {
        return -1;
    }
    if (keylen != IOT_CMFT_SECKEY_LEN) {
        return -1;
    }

    et = time(NULL) + 31536000 + 31536000;
    snprintf(sigstr, sizeof(sigstr) - 1, "%lu\nsha1\nproducts/%s/devices/%s\n" IOT_CMFT_MQTT_VER_STR,
        et, pid, did);
    utils_hmac_sha1(sigstr, strlen(sigstr), digest, keydec, keylen);

    if (mbedtls_base64_encode((unsigned char *)digest_enc, sizeof(digest_enc), &digest_len, 
        (const unsigned char *)digest, IOT_CMFT_SECKEY_LEN) != 0) {
        return -1;
    }
    utils_urlencode(digest_enc, digest_url, sizeof(digest_url));

    snprintf(token, token_max_len - 1,
        "version=" IOT_CMFT_MQTT_VER_STR "&res=products%%2F%s%%2Fdevices%%2F%s&et=%ld&method=sha1&sign=%s",
        pid, did, et, digest_url);

    return 0;
}
