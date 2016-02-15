#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lookup.h"

lookup_t * lookup_init(){
    lookup_t * l;
    l = calloc(1, sizeof(lookup_t));
    l->networks = calloc(10000, sizeof(network_t));
    return l;
}

bool lookup_load(lookup_t * handle, const char * filename){
    FILE *fp;
    char buff[255];
    char * p, * string;
    char de[] = "/\n";
    uint8_t ret;
    struct in_addr addr;
    uint32_t i = 0;

    fp = fopen(filename, "r");
    while(fgets(buff, 255, fp)){
        string = buff;
        //extract the ip part
        p = strsep(&string, de);
        if(inet_pton(AF_INET, p, &addr) != 1){
            printf("Failed\n");
        }
        //extract the mask part
        p = strsep(&string, de);
        ret = strtol(p, NULL, 10);
        handle->networks[i].network = addr;
        handle->networks[i].netmask = ret;
        i++;
    }
    handle->size = i;
    fclose(fp);
    return true;
}

bool lookup_dump(lookup_t * handle){
    char buff[1024];
    for(uint16_t i =0; i < handle->size; i++){
        inet_ntop(AF_INET, &handle->networks[i].network, buff, 1024);
        printf("%s/%d\n", buff, handle->networks[i].netmask);
    }
    return true;
}

bool lookup_search(lookup_t * handle, struct in_addr ip){
    uint16_t i;
    uint16_t index;
    struct in_addr netmask;
    struct in_addr network;
    char buff[1024];
    uint32_t nmask = 0;

    for(i = 0;i < handle->size; i++){
        netmask.s_addr = 0xffffffff >> (32 - handle->networks[i].netmask);
        network.s_addr = ip.s_addr & netmask.s_addr;
        if(nmask >= handle->networks[i].netmask){
            continue;
        }
        //printf("here\n");
        if(handle->networks[i].network.s_addr == network.s_addr){
            nmask = handle->networks[i].netmask;
            index = i + 1;
        }
    }
    if(nmask == 0){
        return false;
    }
    // printf("ip found in  index:%d \n", index);
    // inet_ntop(AF_INET, &network, buff, 1024);
    // printf("%0x %s\n", netmask.s_addr, buff);
    return true;
}

bool lookup_free(lookup_t * handle){
    free(handle->networks);
    free(handle);
    return true;
}
