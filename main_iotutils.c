#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "iotgw.h"
#include "iotgw_utils.h"

void show_usage()
{
    printf("\nUsage: iotutils token|subdev \n"
        "\tiotutils token PRODUCT_ID DEVICE_ID SECRET\n"
        "\tiotutils subdev\n");
    return;
}

int main(int argc, char **argv)
{
    char token[256], *pid, *did, *sec;


    if (argc == 5 && strcmp(argv[1], "token") == 0) {
        /* iotutils token|xxx PRODUCT_ID DEVICE_ID SECRET */
        pid = argv[2];
        did = argv[3];
        sec = argv[4];
        if ((pid == NULL || strlen(pid) != 6) ||
            (did == NULL || strlen(did) != 8) ||
            (sec == NULL || strlen(sec) != 28)) {
            printf("Cracked as PID=%s DID=%s SEC=%s\n", pid, did, sec);
            goto err;
        }
        iot_cmft_get_token_mqtt(pid, did, sec, token, sizeof(token));
        printf("Token: %s\n", token);
    }
    else 
    {
        goto err;
    }

    return 0;

err:
    show_usage();
    return -1;
}





