#ifndef _IOTGW_H_
#define _IOTGW_H_

#include "iotgw_mqtt_client.h"

#define IOTGW_DEFAULT_QOS               1
#define IOTGW_DEFAULT_KEEPALIVE         30

#define IOTGW_CONFIG_FILE      "/Volumes/Apps/MyWork/iotgw/mqttgateway/conf.json"
#define IOTGW_DATA_FILE        "/Volumes/Apps/MyWork/iotgw/mqttgateway/gateway.json"
#define IOTGW_IDSTR_LEN        16
#define IOTGW_SECSTR_LEN       32
#define IOTGW_TOKEN_LEN        128

typedef struct iotgw_dev {
    char pid[IOTGW_IDSTR_LEN];
    char did[IOTGW_IDSTR_LEN];
    char sec[IOTGW_SECSTR_LEN];
    char token[IOTGW_TOKEN_LEN];

    struct iotgw_mqtt_conn mqttConn;
    int OnlineSubDeviceCount;
    int TotalSubDeviceCount;
} iotgw_dev_t;

typedef struct iotgw_subdev {
    struct iotgw_dev *parent;

    char pid[IOTGW_IDSTR_LEN];
    char did[IOTGW_IDSTR_LEN];
    char sec[IOTGW_SECSTR_LEN];
    char token[IOTGW_TOKEN_LEN];
} iotgw_subdev_t;


#endif /* _IOTGW_H_ */