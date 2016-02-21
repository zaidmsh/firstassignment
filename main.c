#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "lookup.h"

#define REPEAT 1000

bool load_networks(lookup_t * l, const char * filename)
{
    FILE *fp;
    char buf[1024];
    char *p, *string;
    char de[] = "/ \n";
    struct in_addr network;
    uint8_t mask;
    uint32_t value;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: %s\n",strerror(errno));
        return false;
    }
    while (fgets(buf, 1024, fp)) {
        string = buf;
        //extract the ip part
        p = strsep(&string, de);
        if(inet_pton(AF_INET, p, &network) != 1){
            printf("Failed\n");
        }
        //extract the mask part
        p = strsep(&string, de);
        mask = strtol(p, NULL, 10);
        //extract the value
        p = strsep(&string, de);
        value = strtol(p, NULL, 10);

        lookup_insert(l, network, mask, value);
    }

    return true;
}

int main()
{
    struct in_addr addr;
    FILE *fp;
    char buf[1024];
    lookup_t *l;
    uint32_t len, i, value;


    l = lookup_init();
    load_networks(l, "networks");
    //lookup_dump(l);
    lookup_build(l);
    //lookup_dump_internal(l);

    for(i = 0; i<REPEAT; i++) {
        fp = fopen("ips", "r");
        if (fp==NULL) {
            fprintf(stderr, "Error: %s\n",strerror(errno));
            return -1;
        }
        while(fgets(buf, 1024, fp)) {
            len = strlen(buf);
            buf[len - 1] = 0;
            if(inet_pton(AF_INET, buf, &addr) != 1) {
                printf("inet_pton: Failed\n");
                break;
            }
            if (lookup_search(l, addr, &value)) {
                // printf("Found: %15s %d\n", buf, value);
            } else {
                printf("NOT Found: %15s\n", buf);
            }
        }
        fclose(fp);
    }

    lookup_free(l);
    return 0;
}
