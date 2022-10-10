/*
 * 	IOTGW MQTT Client
 *
 *  Created on: Sep 22, 2022
 *  Author: Richard Lee
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "MQTTAsync.h"
#include "json-c/json.h"

#include "iotgw_utils.h"
#include "iotgw_mqtt_client.h"
#include "dbg_printf.h"

/*
 * Called by the client library if the client loses its connection to the server.
 * The client application must take
 * appropriate action, such as trying to reconnect or reporting the problem.
 * This function is executed on a separate thread to the one on which the
 * client application is running.
 */
void connectionLost(void *context, char *cause)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	ctx->state &= ~MQTT_CONN_STATE_CONNECTED;
	ctx->state |= MQTT_CONN_STATE_QUIT;

	DBG_PRT("Connection lost and cause as [%s]", cause);
	if (ctx->onConnectionLost)
		(*(ctx->onConnectionLost))(ctx);

	if (ctx->handle) {
		DBG_PRT("Reconnecting to %s in %d seconds", ctx->serverURI, ctx->conn_opts.keepAliveInterval);

		if (ctx->retry_times > 0) {
			conn_opts.keepAliveInterval = ctx->conn_opts.keepAliveInterval;
			conn_opts.cleansession = 1;
			DBG_PRT("Try to reconnect to %s in %d times after %d seconds", ctx->serverURI,
				ctx->retry_times, conn_opts.keepAliveInterval);
			if ((rc = MQTTAsync_connect(ctx->handle, &conn_opts)) != MQTTASYNC_SUCCESS)
			{
				ERR_PRT("Failed to start reconnect, return code %d", rc);
			}
			ctx->retry_times--;
		} else {
			DBG_PRT("Failed to reconnect finally");
		}
	}
}

/*
 * Called by the client library when a new message that matches a client
 * subscription has been received from the server. This function is executed on
 * a separate thread to the one on which the client application is running.
 */
int messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *m)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state |= MQTT_CONN_STATE_MSGARRIVED;

	if (ctx && ctx->onMessageArrived)
		return (*(ctx->onMessageArrived))(ctx, topicName, topicLen, m ? m->payload : NULL, m ? m->payloadlen : 0);
	else
		return 1;
}

/*
 * Called by the client library after the client application has
 * published a message to the server. It indicates that the necessary
 * handshaking and acknowledgements for the requested quality of service (see
 * MQTTAsync_message.qos) have been completed. This function is executed on a
 * separate thread to the one on which the client application is running.
 */
void deliveryComplete(void *context, MQTTAsync_token token)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state |= MQTT_CONN_STATE_DELIVERED;

	if (ctx && ctx->onDeliveryComplete)
		(*(ctx->onDeliveryComplete))(ctx, token);
	return;
}

void onDisconnectFailure(void *context, MQTTAsync_failureData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state &= ~MQTT_CONN_STATE_CONNECTED;
	ctx->state |= MQTT_CONN_STATE_QUIT;
	DBG_PRT("Disconnect failed and FORCED to DISCONNECT");
}

void onDisconnect(void *context, MQTTAsync_successData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state &= ~MQTT_CONN_STATE_CONNECTED;
	ctx->state |= MQTT_CONN_STATE_QUIT;
	DBG_PRT("Successful disconnection");
}

void onPublishFailure(void *context, MQTTAsync_failureData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state &= ~MQTT_CONN_STATE_PUBLISH_SUCCESS;
	ctx->state |= MQTT_CONN_STATE_PUBLISH_FAIL;

	DBG_PRT("Publish message token=%d FAILED as errorcode=%d msg=%s",
		   response->token, response->code, response->message);

	if (ctx && ctx->onPublishFailure && response)
		(*(ctx->onPublishFailure))(ctx, response->token, response->code);
}

/*
 * Called if publish message successfully completes
 */
void onPublishSuccess(void *context, MQTTAsync_successData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state |= MQTT_CONN_STATE_PUBLISH_SUCCESS;
	ctx->state &= ~MQTT_CONN_STATE_PUBLISH_FAIL;

	DBG_PRT("Publish message token=%d CONFIRMED SUCCESS", response->token);

	if (ctx && ctx->onPublishSuccess && response)
		(*(ctx->onPublishSuccess))(ctx,
			response->alt.pub.destinationName,
			response->alt.pub.message.payload, response->alt.pub.message.payloadlen,
			response->token);
}

/*
 * Called if the connect fails
 */
void onConnectFailure(void *context, MQTTAsync_failureData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;
	const char * errmsg[] = {
		"UNKNOWN", 
		"Connection refused: Unacceptable protocol version",
		"Connection refused: Identifier rejected",
		"Connection refused: Server unavailable",
		"Connection refused: Bad user name or password",
		"Connection refused: Not authorized"};

	ctx->state &= ~MQTT_CONN_STATE_CONNECTED;
	ctx->state |= MQTT_CONN_STATE_QUIT;

	DBG_PRT("Connect to %s failed, rc=%d %s", ctx->serverURI, response->code,
		(response->code > 0 && response->code < 6) ? errmsg[response->code] : (response->message ? : "NULL"));
}

/*
 * Called if the connect successfully completes
 */
void onConnect(void *context, MQTTAsync_successData *response)
{
	struct iotgw_mqtt_conn *ctx = (struct iotgw_mqtt_conn *)context;

	ctx->state |= MQTT_CONN_STATE_CONNECTED;

	DBG_PRT("Connected to %s VER=%d successfully as USERNAME=%s CLIENTID=%s",
		   response ? response->alt.connect.serverURI : "",
		   response ? response->alt.connect.MQTTVersion : -1,
		   ctx ? ctx->clientUsername : "",
		   ctx ? ctx->clientId : "");

	if (ctx && ctx->onConnectSuccess)
	{
		(*(ctx->onConnectSuccess))(ctx,
			response ? response->alt.connect.serverURI : NULL,
			response ? response->alt.connect.MQTTVersion : -1);
	}
}

/*
 * Set MQTT Connect Options to defaults
 */
void iotgw_mqtt_setdef(struct iotgw_mqtt_conn *mqttConn)
{
	MQTTAsync_connectOptions opts_def = MQTTAsync_connectOptions_initializer;

	if (mqttConn == NULL)
		return;

	opts_def.keepAliveInterval = IOTGW_DEFAULT_KEEPALIVE;
	opts_def.cleansession = 1;
	opts_def.onSuccess = onConnect;
	opts_def.onFailure = onConnectFailure;
	opts_def.context = mqttConn;
	opts_def.username = mqttConn->clientUsername;
	opts_def.password = mqttConn->clientPassword;	

	memset(mqttConn, 0, sizeof(struct iotgw_mqtt_conn));
	memcpy(&mqttConn->conn_opts, &opts_def, sizeof(MQTTAsync_connectOptions));
	mqttConn->state = MQTT_CONN_STATE_NONE;
	mqttConn->retry_times = IOTGW_RETRY_TIMES;
}

/*
 * Start to connect to MQTT Server
 */
int iotgw_mqtt_connect(struct iotgw_mqtt_conn *mqttConn)
{
	int rc = MQTTASYNC_FAILURE, waitcount;
	MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;

	if (mqttConn == NULL || mqttConn->serverURI[0] == 0 || mqttConn->clientId[0] == 0)
		return MQTTASYNC_FAILURE;

	DBG_PRT("MQTTAsync CONNECTING to %s", mqttConn->serverURI);
	mqttConn->state = MQTT_CONN_STATE_NONE;

	if ((rc = MQTTAsync_create(&mqttConn->handle, mqttConn->serverURI, mqttConn->clientId, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		ERR_PRT("Failed to create MQTT ASYNC Client URI=%s ClientId=%s, return code %d",
			   mqttConn->serverURI, mqttConn->clientId, rc);
		return MQTTASYNC_FAILURE;
	}

	if ((rc = MQTTAsync_setCallbacks(mqttConn->handle, mqttConn, connectionLost, messageArrived, deliveryComplete)) != MQTTASYNC_SUCCESS)
	{
		ERR_PRT("Failed to set callback, return code %d", rc);
		goto ret;
	}

	memcpy(&opts, &mqttConn->conn_opts, sizeof(MQTTAsync_connectOptions));
	opts.onSuccess = onConnect;
	opts.onFailure = onConnectFailure;
	opts.context = mqttConn;
	opts.username = mqttConn->clientUsername;
	opts.password = mqttConn->clientPassword;
	mqttConn->retry_times = IOTGW_RETRY_TIMES;
	if ((rc = MQTTAsync_connect(mqttConn->handle, &opts)) != MQTTASYNC_SUCCESS)
	{
		ERR_PRT("Failed to start connectting to URI=%s, return code %d", mqttConn->serverURI, rc);
		goto ret;
	}

	/* Waiting for connection in other thread */
	waitcount = IOTGW_MQTT_TIMEOUT;
    while (waitcount-- > 0 && (mqttConn->state & MQTT_CONN_STATE_QUIT) == 0) {
        if ((mqttConn->state & MQTT_CONN_STATE_CONNECTED) == 1) {
            break;
		}
        usleep(IOTGW_MQTT_TICKER);
    }
    if (waitcount == 0 && (mqttConn->state & MQTT_CONN_STATE_CONNECTED) != 1) {
		LOG_PRT("Waiting for MQTT CONNECT TIMEOUT");
		rc = MQTTASYNC_FAILURE;
		goto ret;
    }

	return MQTTASYNC_SUCCESS;

ret:
	if (mqttConn->handle)
	{
		MQTTAsync_destroy(&mqttConn->handle);
		mqttConn->handle = NULL;
	}
	mqttConn->state = MQTT_CONN_STATE_NONE;
	return rc;
}

int iotgw_mqtt_disconnect(struct iotgw_mqtt_conn *mqttConn)
{
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc, waitcount;

	if (mqttConn == NULL || mqttConn->handle == NULL)
		return MQTTASYNC_SUCCESS;
	
	if ((mqttConn->state & MQTT_CONN_STATE_CONNECTED) == 0)
		goto ret;
	
	mqttConn->state |= MQTT_CONN_STATE_QUIT;
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = mqttConn;
	if ((rc = MQTTAsync_disconnect(mqttConn->handle, &opts)) != MQTTASYNC_SUCCESS)
	{
		ERR_PRT("Failed to start disconnect, return code %d", rc);
		return MQTTASYNC_FAILURE;
	}

	/* Waiting for dis-connection in other thread */
    waitcount = IOTGW_MQTT_TIMEOUT;
    while (waitcount-- > 0) {
        if ((mqttConn->state & MQTT_CONN_STATE_CONNECTED) == 0) {
            break;
		}
        usleep(IOTGW_MQTT_TICKER);
    }
    if (waitcount == 0 && (mqttConn->state & MQTT_CONN_STATE_CONNECTED) != 0) {
		LOG_PRT("Waiting for MQTT DISCONNECT TIMEOUT and forced to disconnect");
    }

ret:
	if (mqttConn->handle)
	{
		MQTTAsync_destroy(&mqttConn->handle);
		mqttConn->handle = NULL;
	}
	mqttConn->state = MQTT_CONN_STATE_NONE;
	return MQTTASYNC_SUCCESS;
}

/*
 * Publish message to MQTT Server TOPIC
 */
int iotgw_mqtt_publish(struct iotgw_mqtt_conn *mqttConn, char *topic, void *payload, int payloadlen, int qos, int *token)
{
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc = MQTTASYNC_FAILURE;

	if ((mqttConn->state & MQTT_CONN_STATE_CONNECTED) == 0 || (mqttConn->state & MQTT_CONN_STATE_QUIT) == 1)
	{
		LOG_PRT("MQTT Client was NOT CONNECTED or QUITTING as state=0x%08x", mqttConn->state);
		return MQTTASYNC_FAILURE;
	}

	mqttConn->state &= ~MQTT_CONN_STATE_DELIVERED;
	mqttConn->state &= ~MQTT_CONN_STATE_PUBLISH_SUCCESS;
	mqttConn->state &= ~MQTT_CONN_STATE_PUBLISH_FAIL;
	mqttConn->state &= ~MQTT_CONN_STATE_QUIT;
	opts.onSuccess = onPublishSuccess;
	opts.onFailure = onPublishFailure;
	opts.context = mqttConn;
	pubmsg.payload = payload;
	pubmsg.payloadlen = payloadlen;
	pubmsg.qos = qos;
	pubmsg.retained = 0;
	if ((rc = MQTTAsync_sendMessage(mqttConn->handle, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		ERR_PRT("Failed to start sendMessage, return code %d", rc);
		return MQTTASYNC_FAILURE;
	}
	if (token)
		*token = opts.token;
	DBG_PRT("Publish to %s as token=%d", topic, opts.token);

	return rc;
}

