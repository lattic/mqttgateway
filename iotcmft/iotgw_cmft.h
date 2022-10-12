#ifndef _IOTGW_CMFT_H_
#define _IOTGW_CMFT_H_

#include "iotgw_mqtt_client.h"

#define IOTGW_CMFT_SERVER_URI       "tcp://mqtt.iot.cmft.com:30001"

int iotgw_mqtt_init();

int iotgw_mqtt_exit();

void iotgw_mqtt_proc();

int iotgw_cmft_publish_gateway_info(struct iotgw_mqtt_conn *mqttConn, const char *filename, unsigned long waitFinishedMS);

int iotgw_cmft_publish_subdev_info(struct iotgw_mqtt_conn *mqttConn, unsigned long waitFinishedMS);

#endif /* _IOTGW_CMFT_H_ */

