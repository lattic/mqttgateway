#ifndef _IOTGW_UTILS_H_
#define _IOTGW_UTILS_H_

int iot_cmft_get_token_mqtt(const char *pid, const char *did, const char *sec, char *token, unsigned int token_max_len);

int iotgw_load_conf(struct iotgw_dev *dev, const char *filename);

#endif  /* _IOTGW_UTILS_H_ */
