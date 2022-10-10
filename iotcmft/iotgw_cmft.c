#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MQTTAsync.h"
#include "json-c/json.h"

#include "iotgw_cmft.h"
#include "iotgw_mqtt_client.h"
#include "iotgw_utils.h"
#include "dbg_printf.h"

#define IOTGW_CMFT_SERVER_URI       "tcp://mqtt.iot.cmft.com:30001"
#define IOTGW_CMFT_TOKEN_LEN        256

#define IOTGW_DEFAULT_PRODUCT_ID    "104841"
#define IOTGW_DEFAULT_DEVICE_ID     "10654986"
#define IOTGW_DEFAULT_SECRET        "Y2JjOWRhNzI2N2M5ZjdjM2JkZmY="

static struct iotgw_mqtt_conn mqtt_conn;

/* Called if the connect successfully completes */
void onConnectSuccess(struct iotgw_mqtt_conn *mqttConn, char* serverURI, int MQTTVersion);

/* Called when a new message that matches a client subscription has been received from the server. */
int onMessageArrived(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen);

int iotgw_mqtt_init()
{
    char token[IOTGW_CMFT_TOKEN_LEN];
    int retval = 0, waitcount;

    iotgw_mqtt_setdef(&mqtt_conn);
    strncpy(mqtt_conn.serverURI, IOTGW_CMFT_SERVER_URI, sizeof(mqtt_conn.serverURI) - 1);
    strncpy(mqtt_conn.clientUsername, IOTGW_DEFAULT_PRODUCT_ID, sizeof(mqtt_conn.clientUsername) - 1);
    strncpy(mqtt_conn.clientId, IOTGW_DEFAULT_DEVICE_ID, sizeof(mqtt_conn.clientId) - 1);
    iot_cmft_get_token_mqtt(IOTGW_DEFAULT_PRODUCT_ID, IOTGW_DEFAULT_DEVICE_ID, IOTGW_DEFAULT_SECRET, token, sizeof(token));
    strncpy(mqtt_conn.clientPassword, token, sizeof(mqtt_conn.clientPassword) - 1);
    mqtt_conn.onConnectSuccess = onConnectSuccess;
    mqtt_conn.onMessageArrived = onMessageArrived;

    LOG_PRT("Initializing CMFT_MQTT_Gateway as URI=%s PID=%s DID=%s SEC=%s", mqtt_conn.serverURI,
        mqtt_conn.clientUsername, mqtt_conn.clientId, IOTGW_DEFAULT_SECRET);
    DBG_PRT("PWD=%s", mqtt_conn.clientPassword);

    if (iotgw_mqtt_connect(&mqtt_conn) != MQTTASYNC_SUCCESS) {
        retval = -1;
        goto ret;
    }

    retval = 0;

ret:
    return retval;
}

int iotgw_mqtt_exit()
{
    INFO_PRT("Try to DISCONNECT from %s PID=%s DID=%s\n", mqtt_conn.serverURI, mqtt_conn.clientUsername, mqtt_conn.clientId);
    iotgw_mqtt_disconnect(&mqtt_conn);

ret:
    return 0;
}

/* Called if the connect successfully completes */
void onConnectSuccess(struct iotgw_mqtt_conn *mqttConn, char* serverURI, int MQTTVersion)
{
}

void iotgw_mqtt_proc()
{
    char topic[256], payload[2048];
    int waitcount, pubtoken;

    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", IOTGW_DEFAULT_PRODUCT_ID, IOTGW_DEFAULT_DEVICE_ID);
    waitcount = 5;
    while(waitcount-- > 0) {
        snprintf(payload, sizeof(payload) - 1, "{\"params\":{"
            "\"DeviceStatus\":{\"value\":\"%s\"},"
            "\"OnlineSubDeviceCount\":{\"value\":%d}}}",
            "RUNNING", waitcount);

        printf("\nPublish to TOPIC %s PAYLOAD: %s\n", topic, payload);
        iotgw_mqtt_publish(&mqtt_conn, "$sys/" IOTGW_DEFAULT_PRODUCT_ID "/" IOTGW_DEFAULT_DEVICE_ID "/thing/property/post",
            payload, strlen(payload), 1, &pubtoken);
        sleep(5);
    }
}

/* Called when a new message that matches a client subscription has been received from the server. */
int onMessageArrived(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen)
{
    return 1;
}
