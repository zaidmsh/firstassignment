#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "lookup.h"
#define HASHTABLE

guint network_hash(gconstpointer key);
gboolean network_equal(gconstpointer key1, gconstpointer key2);

lookup_t * lookup_init(){
    lookup_t * l;
    l = calloc(1, sizeof(lookup_t));
    l->networks = calloc(10000, sizeof(network_t));
    l->hash = g_hash_table_new_full(network_hash, network_equal, free, NULL);
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
    network_t * network;

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
#ifdef HASHTABLE
        //insert network in hashtable
        network = calloc(1, sizeof(network_t));
        network->network = addr;
        network->netmask = ret;
        g_hash_table_add(handle->hash, network);
#else
        //insert network
        handle->networks[i].network = addr;
        handle->networks[i].netmask = ret;
        i++;
#endif
    }
    handle->size = i;
    fclose(fp);
    printf("Hash size: %d\n", g_hash_table_size(handle->hash));
    return true;
}

bool lookup_dump(lookup_t * handle){
    char buff[1024];
    uint16_t i;
    for(i =0; i < handle->size; i++){
        inet_ntop(AF_INET, &handle->networks[i].network, buff, 1024);
        printf("%s/%d\n", buff, handle->networks[i].netmask);
    }
    return true;
}

bool lookup_search(lookup_t * handle, struct in_addr ip){
#ifdef HASHTABLE
    uint32_t i;
    struct in_addr netmask;
    struct in_addr network;
    network_t key;
    char buff[1024];

    for(i = 32;i > 0; i--){
        netmask.s_addr = htonl(0xffffffff << (32 - i));
        network.s_addr = ip.s_addr & netmask.s_addr;
        key.network = network;
        key.netmask = i;
        if(g_hash_table_lookup_extended(handle->hash, &key, NULL, NULL)){
            inet_ntop(AF_INET, &network, buff, 1000);
            printf("found:%s/%d\n", buff, i);
            return true;
        }
    }
    return false;
#else
    uint16_t i;
    uint16_t index;
    struct in_addr netmask;
    struct in_addr network;
    // char buff[1024];
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
#endif
}

bool lookup_free(lookup_t * handle){
    free(handle->networks);
    g_hash_table_destroy(handle->hash);
    free(handle);
    return true;
}







uint32_t hashfnv1a_32(const void *data, uint32_t nbytes)
{
    if (data == NULL || nbytes == 0) return 0;
    uint8_t  *dp;
    uint32_t h = 0x811C9DC5;

    for (dp = (uint8_t *) data; *dp && nbytes > 0; dp++, nbytes--)
    {
        h ^= *dp;

#ifdef __GNUC__
        h += (h<<1) + (h<<4) + (h<<7) + (h<<8) + (h<<24);
#else
        h *= 0x01000193;
#endif
    }
    return h;
}

guint network_hash(gconstpointer key)
{
    guint hval;

    hval = hashfnv1a_32(key, sizeof(network_t));
    return hval;
}
gboolean network_equal(gconstpointer key1, gconstpointer key2)
{
    network_t *k1;
    network_t *k2;

    k1 = (network_t*)key1;
    k2 = (network_t*)key2;
    return (k1->network.s_addr == k2->network.s_addr) && (k1->netmask == k2->netmask);
}
