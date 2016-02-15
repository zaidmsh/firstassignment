#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include "lookup.h"

int main(){
    struct in_addr addr;
    FILE *fp;
    char buff[255];
    lookup_t * l;
    uint32_t len, i;
    l = lookup_init();
    lookup_load(l, "networks");
    //lookup_dump(l);
    for(i = 0; i<20; i++){
        fp = fopen("ips", "r");
        while(fgets(buff, 255, fp)){
            len =  strlen(buff);
            buff[len - 1] = 0;
            if(inet_pton(AF_INET, buff, &addr) != 1){
                printf("inet_pton: Failed\n");
                break;
            }
            lookup_search(l, addr);
        }
        fclose(fp);
    }
    lookup_free(l);
}
