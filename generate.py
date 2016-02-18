#!/usr/bin/env python
import random
import socket
import struct

def generate_netmask():
    networks = []
    for i in range(33):
        x = (0xffffffff << (32 - i)) & 0xffffffff
        #print '{0:032b}'.format(x)
        print '%08x,' % x
        networks.append(x)
    return networks

def ip_to_string(ip):
    return socket.inet_ntop(socket.AF_INET, struct.pack('!I', ip))

def main():
    fnet = open('networks2', 'w')
    fip = open('ips', 'w')
    netmasks = generate_netmask()
    for i in range(10000):
        ip = random.randint(0xD4000000,0xD4FFFFFF)
        str_ip = socket.inet_ntop(socket.AF_INET, struct.pack('!I', ip))
        netmask = random.randint(8,24)
        network = ip & netmasks[netmask]
        hop = chr(65 + ((network + netmask) % 26) )
        fip.write("%s\n" % str_ip)
        fnet.write("%s/%s %s\n" % (ip_to_string(network), netmask, hop))


if __name__ == '__main__':
    main()
