#!/usr/bin/env python
import random
import socket
import struct

def generate_netmask():
    networks = []
    for i in range(33):
        x = (0xffffffff << (32 - i)) & 0xffffffff
        #print '{0:032b}'.format(x)
        #print '%0x,' % x
        networks.append(x)
    return networks

def ip_to_string(ip):
    return socket.inet_ntop(socket.AF_INET, struct.pack('!I', ip))

def main():
    netmasks = generate_netmask()
    for i in range(10000):
        ip = random.randint(0xD4000000,0xD4FFFFFF)
        str_ip = socket.inet_ntop(socket.AF_INET, struct.pack('!I', ip))
        netmask = random.randint(8,24)
        network = ip & netmasks[netmask]
        print str_ip
        print "%s/%s" % (ip_to_string(network), netmask)
    

if __name__ == '__main__':
    main()
