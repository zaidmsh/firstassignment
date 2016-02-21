#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "lookup.h"
#define HASHTABLE

#define BUF_LEN 1024

guint network_hash(gconstpointer key);
gboolean network_equal(gconstpointer key1, gconstpointer key2);
int network_compare(const void * a, const void * b);
bool create_ranges(lookup_t *);
char * my_atop(uint32_t ip, char * buf, uint32_t buf_len);


lookup_t * lookup_init()
{
    lookup_t * l;
    l = calloc(1, sizeof(lookup_t));
    l->networks = calloc(10000, sizeof(network_t));
    l->hash = g_hash_table_new_full(network_hash, network_equal, free, NULL);
    return l;
}

bool lookup_load(lookup_t * handle, const char * filename)
{
    FILE *fp;
    char buf[255];
    char * p, * string;
    char de[] = "/ \n";
    struct in_addr addr;
    uint8_t mask;
    uint32_t value;
    uint32_t i = 0;
    network_t * network;

    fp = fopen(filename, "r");
    if( fp == NULL){
      fprintf(stderr, "Error: %s\n",strerror(errno));
      return false;
    }
    while(fgets(buf, 255, fp)){//reads a line from the file fp and stores it in buf
        string = buf;
        //extract the ip part
        p = strsep(&string, de);
        if(inet_pton(AF_INET, p, &addr) != 1){
            printf("Failed\n");
        }
        //extract the mask part
        p = strsep(&string, de);
        mask = strtol(p, NULL, 10);
        //extract the value
        p = strsep(&string, de);
        value = strtol(p, NULL, 10);
// #ifdef HASHTABLE
        //insert network in hashtable
        network = calloc(1, sizeof(network_t));
        network->network = ntohl(addr.s_addr);
        network->netmask = mask;
        network->value = value;
        g_hash_table_add(handle->hash, network);
// #else
        //insert network
        handle->networks[i].network = ntohl(addr.s_addr);
        handle->networks[i].netmask = mask;
        handle->networks[i].value = value;
        i++;
// #endif
    }
    handle->size = i;
    fclose(fp);
    // printf("Hash size: %d\n", g_hash_table_size(handle->hash));
    qsort(handle->networks, handle->size, sizeof(network_t), network_compare);
    create_ranges(handle);
    return true;
}

bool lookup_dump(lookup_t * handle){//to print the networks
    char buf[BUF_LEN];
    struct in_addr addr;
    uint16_t i;

    for(i =0; i < handle->size; i++){
        addr.s_addr = htonl(handle->networks[i].network);
        inet_ntop(AF_INET, &addr, buf, BUF_LEN);
        printf("%s/%d %d\n", buf, handle->networks[i].netmask, handle->networks[i].value);
    }
    return true;
}

bool lookup_search(lookup_t * handle, struct in_addr ip){
#ifdef HASHTABLE
    uint32_t i;
    uint32_t int_ip;
    struct in_addr network;
    network_t key;
    char buf[BUF_LEN];

    int_ip = ntohl(ip.s_addr);
    for(i = 32;i > 0; i--){
        key.network = int_ip & (0xffffffff << (32 - i));
        key.netmask = i;
        if(g_hash_table_lookup_extended(handle->hash, &key, NULL, NULL)){
            network.s_addr = htonl(key.network);
            inet_ntop(AF_INET, &network, buf, 1000);
            //printf("found:%s/%d\n", buf, i);
            return true;
        }
    }
    return false;
#else
    uint16_t i;
    uint16_t index;
    uint32_t int_ip;
    struct in_addr network;
    // char buf[BUF_LEN];
    uint32_t nmask = 0;

    int_ip = ntohl(ip.s_addr);
    for(i = 0;i < handle->size; i++){
        if(nmask >= handle->networks[i].netmask){
            continue;
        }
        int_network = int_ip & (0xffffffff >> (32 - handle->networks[i].netmask));
        //printf("here\n");
        if(handle->networks[i].network== int_network){
            nmask = handle->networks[i].netmask;
            index = i + 1;
        }
    }
    if(nmask == 0){
        return false;
    }
    // printf("ip found in  index:%d \n", index);
    // inet_ntop(AF_INET, &network, buf, BUF_LEN);
    // printf("%0x %s\n", netmask.s_addr, buf);
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
    network_t * k;

    k = (network_t *)key;
    hval = hashfnv1a_32(key, sizeof(network_t));
    // hval = (guint)(k->network.s_addr ^ k->netmask);
    return hval;
}
gboolean network_equal(gconstpointer key1, gconstpointer key2)
{
    network_t *k1;
    network_t *k2;

    k1 = (network_t*)key1;
    k2 = (network_t*)key2;
    return (k1->network == k2->network) && (k1->netmask == k2->netmask);
}

int network_compare(const void * a, const void * b)
{
    network_t * n1 = (network_t *) a;
    network_t * n2 = (network_t *) b;

    if(n1->network > n2->network) {
        return 1;
    } else if(n1->network < n2->network) {
        return -1;
    } else {
        return n1->netmask - n2->netmask;
    }
}


#define LOOKUP_START 1
#define LOOKUP_END 2

bool create_ranges(lookup_t * handle)
{
    network_t * l1;//will be using netmask variable to indicate the start of the range or the end of it
    network_t * l2;//will ignore the netmask
    uint32_t network_start;
    uint32_t network_end;
    uint32_t netmask_temp;
    uint32_t value_temp;
    uint32_t i;
    uint32_t stack[32];
    uint32_t stack_index = 0;
    char buf[BUF_LEN];

    uint32_t last_net = 0xffffffff;
    uint32_t last_value = 0;
    uint32_t l2_size = 0;

    index_t * index;
    range_t * range;
    uint32_t j, k, chunk_start, chunk_len, l2_offset;

    // phase 1
    l1 = calloc(handle->size * 2, sizeof(network_t));
    for (i = 0; i < handle->size; i++) {
        network_start = handle->networks[i].network;
        netmask_temp = handle->networks[i].netmask;
        value_temp = handle->networks[i].value;
        if(netmask_temp == 32){
            network_end = network_start;
        } else {
            network_end = network_start | (0xffffffff >> netmask_temp);
        }

        l1[i * 2].network = network_start;
        l1[i * 2].value = value_temp;
        l1[i * 2].netmask = LOOKUP_START;
        l1[i * 2 + 1].network = network_end;
        l1[i * 2 + 1].value = value_temp;
        l1[i * 2 + 1].netmask = LOOKUP_END;
    }
    qsort(l1, handle->size * 2, sizeof(network_t), network_compare);

    // Print L1
    printf("===== L1 ====\n");
    for(i = 0; i < handle->size * 2; i++){
        my_atop(l1[i].network, buf, BUF_LEN);
        printf("%c %s %u\n", l1[i].netmask == 1? '>':'<', buf, l1[i].value);
    }

    // phase 2
    l2 = calloc(handle->size * 2, sizeof(network_t));
    for(i = 0 ; i < (handle->size * 2) - 1; i++) {
        if(l1[i].netmask == LOOKUP_START) {
            stack[stack_index++] = l1[i].value;
            // if same value skip
            if(last_value == l1[i].value) continue;
            last_value = l1[i].value;
            // if same network overwrite
            if (last_net == l1[i].network) l2_size--;
            last_net = l1[i].network;
            // insert network into l2
            l2[l2_size].network = l1[i].network;
            l2[l2_size].value = l1[i].value;
            l2_size++;
        } else {
            stack_index--;
            // if same value skip
            if(last_value == stack[stack_index - 1]) continue;
            last_value = stack[stack_index - 1];
            // if same network overwrite
            if (last_net == l1[i].network) l2_size--;
            last_net = l1[i].network;
            // insert network into l2
            l2[l2_size].network = l1[i].network + 1;
            l2[l2_size].value = stack[stack_index - 1];
            l2_size++;
        }
    }

    // Print L2
    printf("===== L2 ====\n");
    for(i = 0; i < l2_size; i++){
        my_atop(l2[i].network, buf, BUF_LEN);
        printf("%d %s => %u\n", i + 1, buf, l2[i].value);
    }

    printf("===== Last Step ====\n");
    uint32_t index_idx;
    uint32_t range_idx;
    uint32_t offset;

    index = (index_t *)calloc(0xffff, sizeof(index_t));
    range = (range_t *)calloc(l2_size, sizeof(range_t));
    l2_offset = 0;
    last_value = 0;
    range_idx = 0;
    for (index_idx = 0; index_idx <= 0xffff; index_idx++) {
        // find chunk size start
        chunk_start = l2_offset;
        for (j = l2_offset; j < l2_size; j++) {
            if (index_idx != (l2[j].network >> 16)) break;
        }
        chunk_len = j - l2_offset;
        l2_offset = j;
        // Process chunk
        if (chunk_len == 0 ) {
            // Use last
            index[index_idx].offset = last_value;
            index[index_idx].len = 0;
        } else if (chunk_len == 1) {
            // Jump to value
            index[index_idx].offset = l2[chunk_start].value;
            index[index_idx].len = 0;
            last_value = l2[chunk_start].value;
        } else {
            // Jump to range
            index[index_idx].offset = range_idx;
            index[index_idx].len = chunk_len;
            for (k=chunk_start; k<chunk_start+chunk_len; k++) {
                // printf("\t\t %s => %u \n", my_atop(l2[k].network, buf, BUF_LEN), l2[k].value);
                range[range_idx].addr = l2[k].network & 0xffff;
                range[range_idx].value = l2[k].value;
                range_idx++;
                last_value = l2[k].value;
            }
        }
    }
    // Print Last Tables
    for (index_idx = 0; index_idx <= 0xffff; index_idx++) {
        printf("Network: %04x ", index_idx);
        if (index[index_idx].len == 0) {
            printf("[*, %u]", index[index_idx].offset);
        } else {
            for (i=0; i<index[index_idx].len; i++) {
                offset = index[index_idx].offset + i;
                printf("[%04x, %u] ", range[offset].addr, range[offset].value);
            }
        }
        printf("\n");
    }

    free(index);
    free(range);
    free(l1);
    free(l2);
    return true;
}


char * my_atop(uint32_t ip, char * buf, uint32_t buf_len)
{
   struct in_addr addr;
   addr.s_addr = htonl(ip);
   inet_ntop(AF_INET, &addr, buf, buf_len);
   return buf;
}
