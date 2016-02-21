#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <errno.h>

#include "lookup.h"

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
    l->hash = g_hash_table_new_full(network_hash, network_equal, free, NULL);
    l->index = NULL;
    l->range = NULL;
    return l;
}

bool lookup_free(lookup_t * handle)
{
    if (handle->hash) { g_hash_table_destroy(handle->hash); }
    if (handle->index) { free(handle->index); }
    if (handle->range) { free(handle->range); }
    free(handle);
    return true;
}

bool lookup_insert(lookup_t * handle, struct in_addr network, uint32_t netmask, uint32_t value)
{
    network_t key;
    network_t * dkey;

    // Make sure I doesn't exists before insertion
    key.network = ntohl(network.s_addr);
    key.netmask = netmask;
    key.value = value;
    if (g_hash_table_lookup_extended(handle->hash, &key, NULL, NULL)) {
        fprintf(stderr, "Network alread exists\n");
        return false;
    }
    // Insert if not found
    dkey = calloc(1, sizeof(network_t));
    dkey->network = ntohl(network.s_addr);
    dkey->netmask = netmask;
    dkey->value = value;
    g_hash_table_add(handle->hash, dkey);

    return true;
}

//to print the networks
bool lookup_dump(lookup_t * handle){
    GHashTableIter iter;
    network_t * key;
    char buf[BUF_LEN];

    g_hash_table_iter_init (&iter, handle->hash);
    while (g_hash_table_iter_next (&iter, (gpointer)&key, NULL))
    {
        printf("%s/%d %d\n", my_atop(key->network, buf, BUF_LEN), key->netmask, key->value);
    }
    return true;
}

bool lookup_search(lookup_t * handle, struct in_addr ip)
{
    return true;
}

#define LOOKUP_START 1
#define LOOKUP_END 2

bool lookup_build(lookup_t * handle)
{
    GHashTableIter iter;
    network_t * key;

    network_t * l1;//will be using netmask variable to indicate the start of the range or the end of it
    network_t * l2;//will ignore the netmask
    uint32_t network_start;
    uint32_t network_end;
    uint32_t netmask_temp;
    uint32_t value_temp;
    uint32_t i;
    uint32_t stack[32];
    uint32_t stack_index = 0;

    uint32_t last_net = 0xffffffff;
    uint32_t last_value = 0;
    uint32_t l1_size, l2_size = 0;

    index_t * index;
    range_t * range;
    uint32_t j, k, chunk_start, chunk_len, l2_offset;
    uint32_t index_idx, range_idx;

    // Free old indx and range
    if (handle->index) { free(handle->index); }
    if (handle->range) { free(handle->range); }

    // phase 1
    l1_size = g_hash_table_size(handle->hash) * 2;
    l1 = calloc(l1_size, sizeof(network_t));

    g_hash_table_iter_init (&iter, handle->hash);
    i = 0;
    while (g_hash_table_iter_next (&iter, (gpointer)&key, NULL))
    {
        //printf("%s/%d %d\n", my_atop(key->network, buf, BUF_LEN), key->netmask, key->value);
        network_start = key->network;
        netmask_temp = key->netmask;
        value_temp = key->value;
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
        i++;
    }
    qsort(l1, l1_size, sizeof(network_t), network_compare);

#if 0
    // Print L1
    printf("===== L1 ====\n");
    for(i = 0; i < l1_size; i++){
        my_atop(l1[i].network, buf, BUF_LEN);
        printf("%c %s %u\n", l1[i].netmask == 1? '>':'<', buf, l1[i].value);
    }
#endif
    // phase 2
    l2 = calloc(l1_size, sizeof(network_t));
    for(i = 0 ; i < l1_size - 1; i++) {
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
#if 0
    // Print L2
    printf("===== L2 ====\n");
    for(i = 0; i < l2_size; i++){
        my_atop(l2[i].network, buf, BUF_LEN);
        printf("%d %s => %u\n", i + 1, buf, l2[i].value);
    }
#endif

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
    handle->index = index;
    handle->range = range;
    free(l2);
    free(l1);
    return true;
}

bool lookup_dump_internal(lookup_t * handle)
{
    uint32_t index_idx, offset, i;
    index_t * index;
    range_t * range;

    index = handle->index;
    range = handle->range;

    for (index_idx = 0; index_idx <= 0xffff; index_idx++) {
        printf("Network: %04x ", index_idx);
        if (index[index_idx].len == 0) {
            printf("[* => %u]", index[index_idx].offset);
        } else {
            for (i=0; i<index[index_idx].len; i++) {
                offset = index[index_idx].offset + i;
                printf("[%04x => %u] ", range[offset].addr, range[offset].value);
            }
        }
        printf("\n");
    }
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

char * my_atop(uint32_t ip, char * buf, uint32_t buf_len)
{
   struct in_addr addr;
   addr.s_addr = htonl(ip);
   inet_ntop(AF_INET, &addr, buf, buf_len);
   return buf;
}
