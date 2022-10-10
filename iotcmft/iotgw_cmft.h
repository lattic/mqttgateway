#ifndef _IOTGW_CMFT_H_
#define _IOTGW_CMFT_H_

#define IOTGW_CMFT_SERVER_URI       "tcp://mqtt.iot.cmft.com:30001"
#define IOTGW_CMFT_TOKEN_LEN        256
#define IOTGW_CMFT_CONFIG_FILE      "/Volumes/Apps/MyWork/iotgw/mqttgateway/gateway.json"

int iotgw_mqtt_init();

int iotgw_mqtt_exit();

void iotgw_mqtt_proc();

int iotgw_cmft_publish_gateway_info(struct iotgw_mqtt_conn *mqttConn, const char *filename);

int iotgw_cmft_publish_subdev_info(struct iotgw_mqtt_conn *mqttConn);

#endif /* _IOTGW_CMFT_H_ */

