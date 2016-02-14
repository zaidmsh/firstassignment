#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include "lookup.h"

int main(){
    struct sockaddr_in sa;

    FILE *fp;
    char buff[255];
    lookup_t * l;
    uint32_t idx;
    l = lookup_init();
    lookup_load(l, "networks");

    fp = fopen("ips", "r");
    while(fgets(buff, 255, fp)){
        if(inet_pton(AF_INET, buff, &sa.sin_addr) == 1){
            printf("inet_pton: Success\n");
            idx = lookup_search(l, sa.sin_addr);
        }
    }
    
    lookup_free(l);
    fclose(fp);
}
