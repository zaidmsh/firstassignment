#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>

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

uint32_t load_ips(const char * filename, struct in_addr * ips, uint32_t ips_length)
{
    FILE *fp;
    char buf[1024];
    uint32_t i;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: %s\n",strerror(errno));
        return false;
    }
    i = 0;
    while (fgets(buf, 1024, fp)) {
        buf[strlen(buf) - 1] = 0;
        if(inet_pton(AF_INET, buf, &ips[i++]) != 1){
            printf("Failed: %s\n", buf);
        }
        if (i==ips_length) break;
    }
    fclose(fp);
    return i;
}

int main()
{
#define IPS_SIZE 10000
    char buf[1024];
    lookup_t *l;
    uint32_t i, j, value;
    struct timeval t1, t2;
    struct in_addr ips[IPS_SIZE];
    uint32_t ips_count;
    uint32_t delta;


    l = lookup_init();
    load_networks(l, "networks");
    ips_count = load_ips("ips", ips, IPS_SIZE);
    printf("IPs loaded: %d\n", ips_count);
    printf("Repeat count: %d\n", REPEAT);
    //lookup_dump(l);
    lookup_build(l);
    //lookup_dump_internal(l);

    gettimeofday(&t1, NULL);

    for(i = 0; i<REPEAT; i++) {
        for(j=0; j<ips_count; j++) {
            if (lookup_search(l, ips[j], &value)) {
                // printf("Found: %15s %d\n", buf, value);
            } else {
                printf("NOT Found: %15s\n", buf);
            }
        }
    }
    gettimeofday(&t2, NULL);
    delta = (1000000 * t2.tv_sec + t2.tv_usec) - (1000000 * t1.tv_sec + t1.tv_usec);
    printf("Time taken: %u micro second\n", delta);
    printf("Speed: %u op/second\n", (uint32_t) (((float)(ips_count*REPEAT)/delta) * 1000000));

    lookup_free(l);
    return 0;
}
