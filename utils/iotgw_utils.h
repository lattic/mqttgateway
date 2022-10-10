#ifndef _IOTGW_UTILS_H_
#define _IOTGW_UTILS_H_

void utils_hmac_sha1(const char *msg, int msg_len, char *digest, const char *key, int key_len);

int utils_urlencode(const char *src, char *dst, unsigned int dstlen);

int iot_cmft_get_token_mqtt(const char *pid, const char *did, const char *sec, char *token, unsigned int token_max_len);

#endif  /* _IOTGW_UTILS_H_ */
