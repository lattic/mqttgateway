/*
 * 	IOTGW MQTT Client
 *
 *  Created on: Sep 22, 2022
 *  Author: Richard Lee
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "json-c/json.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"

#include "iotgw.h"
#include "iotgw_utils.h"
#include "dbg_printf.h"

#define IOT_CMFT_SECKEY_LEN                     20
#define IOT_CMFT_TOKEN_MQTT_VER_STR             "1"
#define IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE      (64)
#define IOT_CMFT_TOKEN_SHA1_DIGEST_SIZE         (20)

void utils_cmft_hmac_sha1(const char *msg, int msg_len, char *digest, const char *key, int key_len)
{
	mbedtls_sha1_context context;
	unsigned char k_ipad[IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE];    /* inner padding - key XORd with ipad  */
	unsigned char k_opad[IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE];    /* outer padding - key XORd with opad */
	unsigned char out[IOT_CMFT_TOKEN_SHA1_DIGEST_SIZE];
	int i;

	if ((NULL == msg) || (NULL == digest) || (NULL == key)) {
		return;
	}

	if (key_len > IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE) {
		return;
	}

	/* start out by storing key in pads */
	memset(k_ipad, 0, sizeof(k_ipad));
	memset(k_opad, 0, sizeof(k_opad));
	memcpy(k_ipad, key, key_len);
	memcpy(k_opad, key, key_len);

	/* XOR key with ipad and opad values */
	for (i = 0; i < IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/* perform inner SHA */
	mbedtls_sha1_init(&context);                                      /* init context for 1st pass */
	mbedtls_sha1_starts(&context);                                    /* setup context for 1st pass */
	mbedtls_sha1_update(&context, k_ipad, IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE);            /* start with inner pad */
	mbedtls_sha1_update(&context, (unsigned char *)msg, msg_len);    /* then text of datagram */
	mbedtls_sha1_finish(&context, out);                               /* finish up 1st pass */

	/* perform outer SHA */
	mbedtls_sha1_init(&context);                              /* init context for 2nd pass */
	mbedtls_sha1_starts(&context);                            /* setup context for 2nd pass */
	mbedtls_sha1_update(&context, k_opad, IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE);    /* start with outer pad */
	mbedtls_sha1_update(&context, out, IOT_CMFT_TOKEN_SHA1_DIGEST_SIZE);     /* then results of 1st hash */
	mbedtls_sha1_finish(&context, out);                       /* finish up 2nd pass */
	memcpy(digest, out, IOT_CMFT_TOKEN_SHA1_DIGEST_SIZE);
}

int utils_cmft_urlencode(const char *src, char *dst, unsigned int dstlen)
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
    snprintf(sigstr, sizeof(sigstr) - 1, "%lu\nsha1\nproducts/%s/devices/%s\n" IOT_CMFT_TOKEN_MQTT_VER_STR,
        et, pid, did);
    utils_cmft_hmac_sha1(sigstr, strlen(sigstr), digest, keydec, keylen);

    if (mbedtls_base64_encode((unsigned char *)digest_enc, sizeof(digest_enc), &digest_len, 
        (const unsigned char *)digest, IOT_CMFT_SECKEY_LEN) != 0) {
        return -1;
    }
    utils_cmft_urlencode(digest_enc, digest_url, sizeof(digest_url));

    snprintf(token, token_max_len - 1,
        "version=" IOT_CMFT_TOKEN_MQTT_VER_STR "&res=products%%2F%s%%2Fdevices%%2F%s&et=%ld&method=sha1&sign=%s",
        pid, did, et, digest_url);

    return 0;
}

int iotgw_load_conf(struct iotgw_dev *dev, const char *filename)
{
    json_object *jso_root = NULL, *jso_iotmqtt = NULL, *jso_gateway = NULL;
	int retval = -1, fd = -1;
     
	if ((fd = open(filename, O_RDONLY, 0)) < 0)
	{
		ERR_PRT("Unable to find gateway configure file %s: %s", filename, strerror(errno));
		goto ret;
	}
	if ((jso_root = json_object_from_fd(fd)) == NULL) {
        ERR_PRT("Unable to parse JSON gateway file %s", filename);
        goto ret;
    }
    close(fd);
    fd = -1;

    dev->OnlineSubDeviceCount = json_object_get_int(json_object_object_get(jso_root, "OnlineSubDeviceCount"));
    dev->TotalSubDeviceCount = json_object_get_int(json_object_object_get(jso_root, "TotalSubDeviceCount"));

    if (!json_object_object_get_ex(jso_root, "iotmqtt", &jso_iotmqtt)) {
        ERR_PRT("Can't find iotmqtt field in JSON file %s", filename);
        goto ret;
    }
    strncpy(dev->uri, json_object_get_string(json_object_object_get(jso_iotmqtt, "URI")) ? : "", sizeof(dev->uri) - 1);

    if (!json_object_object_get_ex(jso_root, "gateway", &jso_gateway)) {
        ERR_PRT("Can't find gateway field in JSON file %s", filename);
        goto ret;
    }
    strncpy(dev->pid, json_object_get_string(json_object_object_get(jso_gateway, "productId")) ? : "", sizeof(dev->pid) - 1);
    strncpy(dev->did, json_object_get_string(json_object_object_get(jso_gateway, "deviceId")) ? : "", sizeof(dev->did) - 1);
    strncpy(dev->sec, json_object_get_string(json_object_object_get(jso_gateway, "secret")) ? : "", sizeof(dev->sec) - 1);
    strncpy(dev->token, json_object_get_string(json_object_object_get(jso_gateway, "token")) ? : "", sizeof(dev->token) - 1);
    json_object_put(jso_root);
    jso_root = NULL;

    if (strlen(dev->pid) != 6 || strlen(dev->did) != 8 || strlen(dev->sec) != 28) {
        ERR_PRT("Crack configure file for pid/did/sec");
        goto ret;
    }
    if (strlen(dev->token) < 118 ) {
        iot_cmft_get_token_mqtt(dev->pid, dev->did, dev->sec, dev->token, sizeof(dev->token));
    }

    retval = 0;
    
ret:
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    if (jso_root != NULL) {
    	json_object_put(jso_root);
        jso_root = NULL;
    }
    return retval;
}
