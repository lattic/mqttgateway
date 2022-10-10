#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iotgw_utils.h"
#include "iotgw_mqtt_client.h"
#include "iotgw_cmft.h"

#define IOTGW_DEFAULT_PRODUCT_ID    "104841"
#define IOTGW_DEFAULT_DEVICE_ID     "10654986"
#define IOTGW_DEFAULT_SECRET        "Y2JjOWRhNzI2N2M5ZjdjM2JkZmY="

int main(int argc, char **argv)
{
    printf("IOT MQTT Gateway Started\n");

    iotgw_mqtt_init();

    iotgw_mqtt_proc();

    iotgw_mqtt_exit();

    return 0;
}

