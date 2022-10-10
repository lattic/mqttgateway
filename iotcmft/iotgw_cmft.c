#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "MQTTAsync.h"
#include "json-c/json.h"

#include "iotgw_mqtt_client.h"
#include "iotgw_cmft.h"
#include "iotgw_utils.h"
#include "dbg_printf.h"

#define IOTGW_DEFAULT_PRODUCT_ID    "104841"
#define IOTGW_DEFAULT_DEVICE_ID     "10654986"
#define IOTGW_DEFAULT_SECRET        "Y2JjOWRhNzI2N2M5ZjdjM2JkZmY="

static struct iotgw_mqtt_conn mqtt_conn;
int OnlineSubDeviceCount = -1, TotalSubDeviceCount = -1;

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
    int waitcount;

    iotgw_cmft_publish_gateway_info(&mqtt_conn, IOTGW_CMFT_CONFIG_FILE);
    while ((mqtt_conn.state & (MQTT_CONN_STATE_PUBLISH_SUCCESS | MQTT_CONN_STATE_PUBLISH_FAIL)) == 0);

    waitcount = 100;
    while (waitcount-- > 0) {
        OnlineSubDeviceCount++;
        TotalSubDeviceCount++;
        iotgw_cmft_publish_subdev_info(&mqtt_conn);
        while ((mqtt_conn.state & (MQTT_CONN_STATE_PUBLISH_SUCCESS | MQTT_CONN_STATE_PUBLISH_FAIL)) == 0);
    }
}

/* Called when a new message that matches a client subscription has been received from the server. */
int onMessageArrived(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen)
{
    return 1;
}

int iotgw_cmft_publish_gateway_info(struct iotgw_mqtt_conn *mqttConn, const char *filename)
{
    json_object *jso_root = NULL, *jso_industryProperties = NULL, *jso_item = NULL,
        *jso_identifier = NULL, *jso_params = NULL, *jso_post = NULL;
    const char *identifier;
	int retval = -1, waitcount, fd = -1, i, value_type, pubtoken;
    long time_curr;
    char topic[256], payload[2048], tmpstr[256];
     
	if ((fd = open(filename, O_RDONLY, 0)) < 0)
	{
		ERR_PRT("Unable to open gateway file %s: %s", filename, strerror(errno));
		goto ret;
	}
	if ((jso_root = json_object_from_fd(fd)) == NULL) {
        ERR_PRT("Unable to parse JSON gateway file %s", filename);
        goto ret;
    }
    close(fd);
    fd = -1;

    if (!json_object_object_get_ex(jso_root, "industryProperties", &jso_industryProperties) ||
        !json_object_is_type(jso_industryProperties, json_type_array) ||
        json_object_array_length(jso_industryProperties) <= 0) {
        ERR_PRT("Can't find industryProperties array field in JSON file %s", filename);
        goto ret;
    }

    /* Generate Gateway Device Information based on file gateway.json */
    time_curr = time(NULL);
    snprintf(tmpstr, sizeof(tmpstr) - 1, "%lu", time_curr);

    jso_post = json_object_new_object();
    json_object_object_add(jso_post, "id", json_object_new_string(tmpstr));
    json_object_object_add(jso_post, "version", json_object_new_string("1.0"));
    jso_params = json_object_new_object();
    json_object_object_add(jso_post, "params", jso_params);
    if (!jso_post || !jso_params) {
        ERR_PRT("FAIL: Not enough memory for JSON objects");
        goto ret;
    }

    /* For dynamic global variables OnlineSubDeviceCount and TotalSubDeviceCount */
    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(OnlineSubDeviceCount));
    json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
    json_object_object_add(jso_params, "OnlineSubDeviceCount", jso_identifier);

    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(TotalSubDeviceCount));
    json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
    json_object_object_add(jso_params, "TotalSubDeviceCount", jso_identifier);

    /* For const information from JSON file */
    for (i = 0; i < json_object_array_length(jso_industryProperties); i++) {
        if ((jso_item = json_object_array_get_idx(jso_industryProperties, i)) == NULL)
            continue;
        identifier = json_object_get_string(json_object_object_get(jso_item, "identifier"));
        if (strcmp(identifier, "OnlineSubDeviceCount") == 0 || strcmp(identifier, "TotalSubDeviceCount") == 0)
            continue;
        
        value_type = json_object_get_int(json_object_object_get(jso_item, "type"));
        jso_identifier = json_object_new_object();
        if (value_type == 3) {
            /* CMFT IOT type=3 for Integer */
            json_object_object_add(jso_identifier, "value",
                json_object_new_int(json_object_get_int(json_object_object_get(jso_item, "value"))));
        } else if (value_type == 1) {
            /* CMFT IOT type=3 for String */
            json_object_object_add(jso_identifier, "value", 
                json_object_new_string(json_object_get_string(json_object_object_get(jso_item, "value"))));
        } else {
            ERR_PRT("Unknown CMFT IOT type=%d", value_type);
        }
        json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
        json_object_object_add(jso_params, identifier, jso_identifier);
    }

    /* POST to CMFT IOT */
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", IOTGW_DEFAULT_PRODUCT_ID, IOTGW_DEFAULT_DEVICE_ID);
    strncpy(payload, json_object_to_json_string(jso_post), sizeof(payload));
    DBG_PRT("Publish to TOPIC %s PAYLOAD: %s", topic, payload);
    iotgw_mqtt_publish(mqttConn, "$sys/" IOTGW_DEFAULT_PRODUCT_ID "/" IOTGW_DEFAULT_DEVICE_ID "/thing/property/post",
            payload, strlen(payload), 1, &pubtoken);

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
    if (jso_post != NULL) {
    	json_object_put(jso_post);
        jso_post = NULL;
    }
    return retval;
}

int iotgw_cmft_publish_subdev_info(struct iotgw_mqtt_conn *mqttConn)
{
    json_object *jso_identifier = NULL, *jso_params = NULL, *jso_post = NULL;
	int retval = -1, pubtoken;
    long time_curr;
    char topic[256], payload[2048], tmpstr[256];
     
    /* Generate Gateway Device Information based on file gateway.json */
    time_curr = time(NULL);
    snprintf(tmpstr, sizeof(tmpstr) - 1, "%lu", time_curr);

    jso_post = json_object_new_object();
    json_object_object_add(jso_post, "id", json_object_new_string(tmpstr));
    json_object_object_add(jso_post, "version", json_object_new_string("1.0"));
    jso_params = json_object_new_object();
    json_object_object_add(jso_post, "params", jso_params);
    if (!jso_post || !jso_params) {
        ERR_PRT("FAIL: Not enough memory for JSON objects");
        goto ret;
    }

    /* For dynamic global variables OnlineSubDeviceCount and TotalSubDeviceCount */
    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(OnlineSubDeviceCount));
    json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
    json_object_object_add(jso_params, "OnlineSubDeviceCount", jso_identifier);

    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(TotalSubDeviceCount));
    json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
    json_object_object_add(jso_params, "TotalSubDeviceCount", jso_identifier);

    /* POST to CMFT IOT */
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", IOTGW_DEFAULT_PRODUCT_ID, IOTGW_DEFAULT_DEVICE_ID);
    strncpy(payload, json_object_to_json_string(jso_post), sizeof(payload));
    DBG_PRT("Publish to TOPIC %s PAYLOAD: %s", topic, payload);
    iotgw_mqtt_publish(mqttConn, "$sys/" IOTGW_DEFAULT_PRODUCT_ID "/" IOTGW_DEFAULT_DEVICE_ID "/thing/property/post",
            payload, strlen(payload), 1, &pubtoken);

    retval = 0;

ret:
    if (jso_post != NULL) {
    	json_object_put(jso_post);
        jso_post = NULL;
    }
    return retval;
}
