#ifndef _IOTGW_MQTT_CLIENT_H_
#define _IOTGW_MQTT_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MQTTAsync.h"

#define IOTGW_DEFAULT_QOS               1
#define IOTGW_DEFAULT_KEEPALIVE         30
#define IOTGW_MQTT_TICKER               100000  /* MQTT Ticker us */
#define IOTGW_MQTT_TIMEOUT              50      /* For ticker period */
#define IOTGW_RETRY_TIMES               3
#define IOTGW_SERVERURI_LENGTH          256
#define IOTGW_CLIENTID_LENGTH           256
#define IOTGW_USERNAME_LENGTH           256
#define IOTGW_PASSWORD_LENGTH           256

enum {
    MQTT_CONN_STATE_NONE = 0, 
    MQTT_CONN_STATE_CONNECTED = 0x01, 
    MQTT_CONN_STATE_MSGARRIVED = 0x02, 
    MQTT_CONN_STATE_DELIVERED = 0x04,
    MQTT_CONN_STATE_PUBLISH_SUCCESS = 0x08,
    MQTT_CONN_STATE_PUBLISH_FAIL = 0x10,

    MQTT_CONN_STATE_QUIT = 0x80
};


typedef struct iotgw_mqtt_conn {
    MQTTAsync handle;
	MQTTAsync_connectOptions conn_opts;

    unsigned int state;
    int retry_times;

    char serverURI[IOTGW_SERVERURI_LENGTH];
    char clientId[IOTGW_CLIENTID_LENGTH];
    char clientUsername[IOTGW_USERNAME_LENGTH];
    char clientPassword[IOTGW_PASSWORD_LENGTH];

    /* Called if the client loses its connection to the server. */
    void (*onConnectionLost)(struct iotgw_mqtt_conn *mqttConn);
    /* Called when a new message that matches a client subscription has been received from the server. */
    int (*onMessageArrived)(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen);
    /* Called after the client application has published a message to the server. It indicates that the necessary handshaking and acknowledgements for the requested quality of service have been completed. */
    void (*onDeliveryComplete)(struct iotgw_mqtt_conn *mqttConn, MQTTAsync_token token);

    /* Called if the connect successfully completes */
    void (*onConnectSuccess)(struct iotgw_mqtt_conn *mqttConn, char* serverURI, int MQTTVersion);
    /* Called if the connect fails */
    void (*onConnectFailure)(struct iotgw_mqtt_conn *mqttConn, MQTTAsync_successData* response);

    /* Called if publish message successfully completes */
    void (*onPublishSuccess)(struct iotgw_mqtt_conn *mqttConn, char * topic, void * payload, int payloadlen, int token);
    /* Called if publish message fails */
    void (*onPublishFailure)(struct iotgw_mqtt_conn *mqttConn, int token, int code);

} iotgw_mqtt_conn_t;

/*
 * Start to connect to MQTT Server
 */
int iotgw_mqtt_connect(struct iotgw_mqtt_conn *mqttConn);

/*
 * Try to disconnect from MQTT Server
 */
int iotgw_mqtt_disconnect(struct iotgw_mqtt_conn *mqttConn);

/* 
 * Set MQTT Connect Options to defaults
 */
void iotgw_mqtt_setdef(struct iotgw_mqtt_conn *mqttConn);

/*
 * Publish message to MQTT Server TOPIC
 */
int iotgw_mqtt_publish(struct iotgw_mqtt_conn *mqttConn, char *topic, void* payload, int payloadlen, int qos, int * token);

#endif /* _IOTGW_MQTT_CLIENT_H_ */
