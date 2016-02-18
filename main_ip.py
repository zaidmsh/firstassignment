#!/usr/bin/env python

import socket
import struct

def aton(ip):
    (l,) = struct.unpack('!L', socket.inet_aton(ip))
    return l

def ntoa(l):
    addr = struct.pack('!L', l)
    ip = socket.inet_ntoa(addr)
    return ip

ranges = [
    ('0.0.0.0', 0, 'A'),
    ('1.0.0.0', 8, 'B'),
    ('1.2.0.0', 16, 'C'),
    ('1.2.3.0', 24, 'D'),
    ('1.2.4.5', 32, 'C'),
]
ranges = [
    ('0.0.0.0', 0, 'A'),
    ('1.0.0.0', 8, 'B'),
    ('1.255.0.0', 16, 'C'),
    ('1.255.255.0', 24, 'D'),
]
wranges = [
    ('0.0.0.0', 0, 'A'),
    ('1.0.0.0', 8, 'B'),
    ('1.0.0.0', 16, 'C'),
    ('1.0.0.0', 24, 'D'),
]
# Sort input array
ranges = sorted(ranges, key=lambda x: (x[0], x[1]))


l1 = []
for net in ranges:
    network, mask, hop = net
    network_start = aton(network)
    network_end = network_start | (0xffffffff >> mask)
    l1.append((network_start, hop, '>'))
    l1.append((network_end, hop, '<'))

l1 = sorted(l1, key=lambda x: x[0] )

last_hop = None
l2 = []
stack = []
for i in l1[:-1]:
    if i[2] == '>':
        stack.append(i[1])
        hop = i[1]
        if hop == last_hop: continue
        last_hop = hop
        if l2 and l2[-1][0] == i[0]:
            l2.pop()
        l2.append((i[0], hop))

    else:
        stack.pop()
        hop = stack[-1]
        if hop == last_hop: continue
        last_hop = hop
        if l2[-1][0] == i[0] + 1:
            l2.pop()
        l2.append((i[0] + 1, hop))

print "========= Network ========="
for i in l2:
    print "%-15s %s" % (ntoa(i[0]), i[1])
