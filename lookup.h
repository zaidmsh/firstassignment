#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>

struct lookup_s;
typedef struct lookup_s lookup_t;

lookup_t * lookup_init();
bool lookup_insert(lookup_t * handle, struct in_addr network, uint32_t netmask, uint32_t value);
bool lookup_build(lookup_t * handle);
bool lookup_dump(lookup_t * handle);
bool lookup_dump_internal(lookup_t * handle);
bool lookup_search(lookup_t * handle, struct in_addr ip, uint32_t * value);
bool lookup_free(lookup_t * handle);
