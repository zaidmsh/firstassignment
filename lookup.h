#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>


struct network_s {
    struct in_addr network;
    uint32_t netmask;
    char hop;
};
typedef struct network_s network_t;

struct lookup_s {
    uint32_t size;
    network_t * networks;
    GHashTable * hash;
};
typedef struct lookup_s lookup_t;


lookup_t * lookup_init();
bool lookup_load(lookup_t * handle, const char * filename);
bool lookup_dump(lookup_t * handle);
bool lookup_search(lookup_t * handle, struct in_addr ip);
bool lookup_free(lookup_t * handle);
