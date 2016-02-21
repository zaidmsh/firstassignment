#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "lookup.h"
#define HASHTABLE


guint network_hash(gconstpointer key);
gboolean network_equal(gconstpointer key1, gconstpointer key2);
int network_compare(const void * a, const void * b);
bool create_ranges(lookup_t *);
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
    char de[] = "/ \n";
    uint8_t mask;
    char hop;
    struct in_addr addr;
    uint32_t i = 0;
    network_t * network;

    fp = fopen(filename, "r");
    if( fp == NULL){
      fprintf(stderr, "Error: %s\n",strerror(errno));
      return false;
    }
    while(fgets(buff, 255, fp)){//reads a line from the file fp and stores it in buff
        string = buff;
        //extract the ip part
        p = strsep(&string, de);
        if(inet_pton(AF_INET, p, &addr) != 1){
            printf("Failed\n");
        }
        //extract the mask part
        p = strsep(&string, de);
        mask = strtol(p, NULL, 10);
        //extract the hop
        p = strsep(&string, de);
        hop = p[0];
// #ifdef HASHTABLE
        //insert network in hashtable
        network = calloc(1, sizeof(network_t));
        network->network = ntohl(addr.s_addr);
        network->netmask = mask;
        network->hop = hop;
        g_hash_table_add(handle->hash, network);
// #else
        //insert network
        handle->networks[i].network = ntohl(addr.s_addr);
        handle->networks[i].netmask = mask;
        handle->networks[i].hop = hop;
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
    char buff[1024];
    struct in_addr addr;
    uint16_t i;

    for(i =0; i < handle->size; i++){
        addr.s_addr = htonl(handle->networks[i].network);
        inet_ntop(AF_INET, &addr, buff, 1024);
        printf("%s/%d %c\n", buff, handle->networks[i].netmask, handle->networks[i].hop);
    }
    return true;
}

bool lookup_search(lookup_t * handle, struct in_addr ip){
#ifdef HASHTABLE
    uint32_t i;
    uint32_t int_ip;
    struct in_addr network;
    network_t key;
    char buff[1024];

    int_ip = ntohl(ip.s_addr);
    for(i = 32;i > 0; i--){
        key.network = int_ip & (0xffffffff << (32 - i));
        key.netmask = i;
        if(g_hash_table_lookup_extended(handle->hash, &key, NULL, NULL)){
            network.s_addr = htonl(key.network);
            inet_ntop(AF_INET, &network, buff, 1000);
            //printf("found:%s/%d\n", buff, i);
            return true;
        }
    }
    return false;
#else
    uint16_t i;
    uint16_t index;
    uint32_t int_ip;
    struct in_addr network;
    // char buff[1024];
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
    char hop_temp;
    uint32_t i;
    char stack[32];
    uint32_t stack_index = 0;
    char buff[1024];

    // phase 1
    l1 = calloc(handle->size * 2, sizeof(network_t));
    for (i = 0; i < handle->size; i++) {
        network_start = handle->networks[i].network;
        netmask_temp = handle->networks[i].netmask;
        hop_temp = handle->networks[i].hop;
        if(netmask_temp == 32){
            network_end = network_start;
        } else {
            network_end = network_start | (0xffffffff >> netmask_temp);
        }

        l1[i * 2].network = network_start;
        l1[i * 2].hop = hop_temp;
        l1[i * 2].netmask = LOOKUP_START;
        l1[i * 2 + 1].network = network_end;
        l1[i * 2 + 1].hop = hop_temp;
        l1[i * 2 + 1].netmask = LOOKUP_END;
    }
    qsort(l1, handle->size * 2, sizeof(network_t), network_compare);

    for(i = 0; i < handle->size * 2; i++){
        inet_ntop(AF_INET, &l1[i].network, buff, 1024);
        printf("%c %s %c\n", l1[i].netmask == 1? '>':'<', buff, l1[i].hop);
    }
    printf("==========================================================\n");

    // phase 2
    l2 = calloc(handle->size * 2, sizeof(network_t));
    uint32_t last_net = 0xffffffff;
    char last_hop = '\0';
    uint32_t l2_size = 0;

    for(i = 0 ; i < (handle->size * 2) - 1; i++) {
        if(l1[i].netmask == LOOKUP_START) {
            stack[stack_index++] = l1[i].hop;
            // if same hop skip
            if(last_hop == l1[i].hop) continue;
            last_hop = l1[i].hop;
            // if same network overwrite
            if (last_net == l1[i].network) l2_size--;
            last_net = l1[i].network;
            // insert network into l2
            l2[l2_size].network = l1[i].network;
            l2[l2_size].hop = l1[i].hop;
            l2_size++;
        } else {
            stack_index--;
            // if same hop skip
            if(last_hop == stack[stack_index - 1]) continue;
            last_hop = stack[stack_index - 1];
            // if same network overwrite
            if (last_net == l1[i].network) l2_size--;
            last_net = l1[i].network;
            // insert network into l2
            l2[l2_size].network = l1[i].network + 1;
            l2[l2_size].hop = stack[stack_index - 1];
            l2_size++;
        }
    }
    // print l2
    struct in_addr addr;
    for(i = 0; i < l2_size; i++){
        addr.s_addr = htonl(l2[i].network);
        inet_ntop(AF_INET, &addr, buff, 1024);
        printf("%d %s %c\n", i + 1, buff, l2[i].hop);
    }

    //Constructing the index and range tables
    index_t * index;
    range_t * range;
    uint32_t start_high, end_high, j;
    index = (index_t *)calloc(0xffff, sizeof(index_t));
    range = (range_t *)calloc(l2_size, sizeof(range_t));
    uint32_t chunk_len, l2_offset = 0;
    last_hop = '\0';

    for (i = 0; i <= 0xffff; i++) {
        printf("%04x\n",i);
        // find chunk size start
        for (j = l2_offset; j < l2_size; j++) {
            if (i != (l2[j].network >> 16)) break;
            printf("\t%04x\n", l2[j].network & 0xffff);
        }
        chunk_len = j - l2_offset;
        l2_offset = j;
        // print
        if (chunk_len == 0 ) {
            printf("use Last: %c\n", last_hop);
        } else if (chunk_len == 1) {
            printf("Jump to hop:%s\n", l2[i].hop);
        } else {
            printf("jump to range table:%s\n", );
        }
        // index[i].offset = 0;
        // index[i].len = chunk_len;
    }


    free(index);
    free(range);
    free(l1);
    free(l2);
    return true;
}
