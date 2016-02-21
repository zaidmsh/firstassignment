#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>

struct network_s {
    uint32_t network;
    uint32_t netmask;
    uint16_t value;
};
typedef struct network_s network_t;

struct index_s {
    uint16_t offset;
    uint16_t len;
};
typedef struct index_s index_t;

struct range_s {
    uint16_t addr;
    uint16_t value;
};
typedef struct range_s range_t;

struct lookup_s {
    GHashTable * hash;
    index_t * index;
    range_t * range;
};

guint network_hash(gconstpointer key);
gboolean network_equal(gconstpointer key1, gconstpointer key2);
int network_compare(const void * a, const void * b);
char * my_atop(uint32_t ip, char * buf, uint32_t buf_len);
