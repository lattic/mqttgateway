#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iotgw_utils.h"

#if 0
$sys/104841/10654986/thing/property/post
{
  "params": {
    "DeviceStatus": { "value": "RUNNING"}
  }
}
#endif


int main(int argc, char **argv)
{
    char token[256];

    printf("IOT MQTT Gateway Utils\n");
    iot_cmft_get_token_mqtt("104841", "10654986", "Y2JjOWRhNzI2N2M5ZjdjM2JkZmY=", token, sizeof(token));
    printf("Token: %s\n", token);
    return 0;
}




