#ifndef _IOTGW_UTILS_H_
#define _IOTGW_UTILS_H_

#define IOT_CMFT_SECKEY_LEN                     20
#define IOT_CMFT_TOKEN_MQTT_VER_STR             "1"
#define IOT_CMFT_TOKEN_SHA1_KEY_IOPAD_SIZE      (64)
#define IOT_CMFT_TOKEN_SHA1_DIGEST_SIZE         (20)

int iot_cmft_get_token_mqtt(const char *pid, const char *did, const char *sec, char *token, unsigned int token_max_len);

#endif  /* _IOTGW_UTILS_H_ */
