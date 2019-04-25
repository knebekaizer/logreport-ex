#!/usr/bin/env python3

import random, sys

import ipaddress
from ipaddress import IPv4Address, IPv6Address, ip_network


registry = []

for line in open("../c5.txt"):
    name, addr = line.split()
    registry.append( (name, ip_network(addr)) )

#print(registry)

unknown = 0
report = {}

#print(report)

lines = 0
for line in open("../iplog.dat"):
    addr, bytes = line.split()
    bytes = int(bytes)
    if not bytes: continue
    ip = ip_network(addr)
    best = None
    for pair in registry:
        if pair[1].supernet_of(ip):
            if not best or best[1].supernet_of(pair[1]):
                best = pair
    if best:
        if best[0] not in report:
            report[best[0]] = bytes
        else:
            report[best[0]] += bytes
    else:
        unknown += bytes
    lines += 1
    print("lines: " + str(lines))


for p in sorted(report):
    print("%s\t%d" % (p, report[p]))
print("%s\t%d" % ('Unknown', unknown))