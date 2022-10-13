#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "MQTTAsync.h"
#include "json-c/json.h"

#include "iotgw.h"
#include "iotgw_cmft.h"
#include "iotgw_mqtt_client.h"
#include "iotgw_utils.h"
#include "dbg_printf.h"

static struct iotgw_dev gwdev;

/* Called if the connect successfully completes */
void onConnectSuccess(struct iotgw_mqtt_conn *mqttConn, char* serverURI, int MQTTVersion);

/* Called when a new message that matches a client subscription has been received from the server. */
int onMessageArrived(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen);

int iotgw_mqtt_init()
{
    char token[IOTGW_TOKEN_LEN];
    int retval = -1, waitcount;
    struct iotgw_mqtt_conn *mqttConn = &gwdev.mqttConn;

    memset(&gwdev, 0, sizeof(struct iotgw_dev));
    mqttConn = &gwdev.mqttConn;
    if (iotgw_load_conf(&gwdev, IOTGW_CONFIG_FILE) != 0) {
        ERR_PRT("Fail to load gateway config from " IOTGW_CONFIG_FILE);
        goto ret;
    }

    /* Initialize MQTT Connection */
    iotgw_mqtt_setdef(mqttConn);
    strncpy(mqttConn->serverURI, gwdev.uri, sizeof(mqttConn->serverURI) - 1);
    strncpy(mqttConn->clientUsername, gwdev.pid, sizeof(mqttConn->clientUsername) - 1);
    strncpy(mqttConn->clientId, gwdev.did, sizeof(mqttConn->clientId) - 1);
    strncpy(mqttConn->clientPassword, gwdev.token, sizeof(mqttConn->clientPassword) - 1);
    mqttConn->onConnectSuccess = onConnectSuccess;
    mqttConn->onMessageArrived = onMessageArrived;

    LOG_PRT("Initializing CMFT_MQTT_Gateway as URI=%s PID=%s DID=%s SEC=%s", mqttConn->serverURI,
        gwdev.pid, gwdev.did, gwdev.sec);
    DBG_PRT("PWD=%s", mqttConn->clientPassword);

    if (iotgw_mqtt_connect(mqttConn) != MQTTASYNC_SUCCESS) {
        retval = -1;
        goto ret;
    }

    retval = 0;

ret:
    return retval;
}

int iotgw_mqtt_exit()
{
    DBG_PRT("Try to DISCONNECT from %s PID=%s DID=%s", gwdev.mqttConn.serverURI, gwdev.mqttConn.clientUsername, gwdev.mqttConn.clientId);
    iotgw_mqtt_disconnect(&gwdev.mqttConn);
    return 0;
}

void iotgw_mqtt_proc()
{
    struct iotgw_mqtt_conn *mqttConn = &gwdev.mqttConn;
    int waitcount, i;

    if (!MQTTAsync_isConnected(mqttConn->handle))
        return;

    iotgw_cmft_publish_gateway_info(mqttConn, IOTGW_DATA_FILE, 0);

#if 0

    for (i = 0; i < 30; i++) {
        waitcount = 50;
        while (waitcount-- > 0) {
            gwdev.OnlineSubDeviceCount++;
            gwdev.TotalSubDeviceCount++;
            iotgw_cmft_publish_subdev_info(mqttConn, 0);
        }
        iotgw_mqtt_waitForAllCompletion(mqttConn->handle, IOTGW_MQTT_TIMEOUT * 1000);

        sleep(15);
    }
#endif

    for (i = 0; i < 100; i++) {
        gwdev.OnlineSubDeviceCount++;
        gwdev.TotalSubDeviceCount++;
        
        INFO_PRT("\nPublish gateway dynamic information for OnlineSubDeviceCount=%d TotalSubDeviceCount=%d", gwdev.OnlineSubDeviceCount, gwdev.TotalSubDeviceCount);
        iotgw_cmft_publish_gateway_info(mqttConn, IOTGW_DATA_FILE, 0);
        iotgw_cmft_publish_subdev_info(mqttConn, 0);
        iotgw_mqtt_waitForAllCompletion(mqttConn->handle, IOTGW_MQTT_TIMEOUT * 1000);

        sleep(15);
    }


}

int iotgw_mqtt_subscribe()
{
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;
#if 0
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
#endif

    return 0;
}

/* Called if the connect successfully completes, Such as subscribe should be implemented */
void onConnectSuccess(struct iotgw_mqtt_conn *mqttConn, char* serverURI, int MQTTVersion)
{
    iotgw_mqtt_subscribe();
}

/* Called when a new message that matches a client subscription has been received from the server. */
int onMessageArrived(struct iotgw_mqtt_conn *mqttConn, char* topicName, int topicLen, void* payload, int payloadlen)
{
    return 1;
}

int iotgw_cmft_publish_gateway_info(struct iotgw_mqtt_conn *mqttConn, const char *filename, unsigned long waitFinishedMS)
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

    /* For dynamic message such as global variables OnlineSubDeviceCount and TotalSubDeviceCount */
    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(gwdev.OnlineSubDeviceCount));
    json_object_object_add(jso_identifier, "time", json_object_new_int64(time_curr));
    json_object_object_add(jso_params, "OnlineSubDeviceCount", jso_identifier);

    jso_identifier = json_object_new_object();
    json_object_object_add(jso_identifier, "value", json_object_new_int(gwdev.TotalSubDeviceCount));
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
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", gwdev.pid, gwdev.did);
    strncpy(payload, json_object_to_json_string(jso_post), sizeof(payload));

    DBG_PRT("Publish TOPIC: %s PAYLOAD: %s", topic, payload);
    if (iotgw_mqtt_publish(mqttConn, topic, payload, strlen(payload), IOTGW_DEFAULT_QOS, &pubtoken) != MQTTASYNC_SUCCESS) {
        ERR_PRT("Fail to publish gateway information");
        goto ret;
    }
    if (waitFinishedMS != 0) {
        MQTTAsync_waitForCompletion(mqttConn->handle, pubtoken, waitFinishedMS);
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
    if (jso_post != NULL) {
    	json_object_put(jso_post);
        jso_post = NULL;
    }
    return retval;
}

int iotgw_cmft_publish_subdev_info(struct iotgw_mqtt_conn *mqttConn, unsigned long waitFinishedMS)
{
	int pubtoken;
    long time_curr;
    char topic[256], payload[2048];
     
    /* Generate Sub-Device Information */
    time_curr = time(NULL);
    snprintf(payload, sizeof(payload) - 1, "{\"id\":\"%lu\",\"version\":\"1.0\",\"params\":{"
        "\"OnlineSubDeviceCount\":{\"value\":%d,\"time\":%lu},"
        "\"TotalSubDeviceCount\":{\"value\":%d,\"time\":%lu}" "}}",
        time_curr,
        gwdev.OnlineSubDeviceCount, time_curr,
        gwdev.TotalSubDeviceCount, time_curr);

    /* POST to CMFT IOT */
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", gwdev.pid, gwdev.did);
    DBG_PRT("Publish TOPIC: %s PAYLOAD: %s", topic, payload);
    if (iotgw_mqtt_publish(mqttConn, topic, payload, strlen(payload), IOTGW_DEFAULT_QOS, &pubtoken) != MQTTASYNC_SUCCESS) {
        ERR_PRT("Fail to publish subdev information");
        return -1;
    }
    if (waitFinishedMS != 0) {
        MQTTAsync_waitForCompletion(mqttConn->handle, pubtoken, waitFinishedMS);
    }
    return 0;

}
