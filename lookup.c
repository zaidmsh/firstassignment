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
    return true;
}
bool lookup_free(lookup_t * handle){
    free(handle->networks);
    free(handle);
    return true;
}
