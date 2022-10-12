#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iotgw.h"
#include "iotgw_mqtt_client.h"
#include "iotgw_utils.h"
#include "iotgw_cmft.h"
#include "dbg_printf.h"

int main(int argc, char **argv)
{
    INFO_PRT("IOT MQTT Gateway Started");

    iotgw_mqtt_init();

    iotgw_mqtt_proc();

    iotgw_mqtt_exit();
    INFO_PRT("Exit IOT MQTT Gateway Successfully");

    return 0;
}

