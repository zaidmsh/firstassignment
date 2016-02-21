#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>


struct network_s {
    uint32_t network;
    uint32_t netmask;
    uint32_t value;
};
typedef struct network_s network_t;

struct index_s {
    uint16_t offset;
    uint16_t len;
};
typedef struct index_s index_t;

struct range_s {
    uint16_t addr;
    uint32_t value;
};
typedef struct range_s range_t;

struct lookup_s {
    GHashTable * hash;
    index_t * index;
    range_t * range;
};
typedef struct lookup_s lookup_t;

lookup_t * lookup_init();
bool lookup_insert(lookup_t * handle, struct in_addr network, uint32_t netmask, uint32_t value);
bool lookup_build(lookup_t * handle);
bool lookup_dump(lookup_t * handle);
bool lookup_dump_internal(lookup_t * handle);
bool lookup_search(lookup_t * handle, struct in_addr ip, uint32_t * value);
bool lookup_free(lookup_t * handle);
