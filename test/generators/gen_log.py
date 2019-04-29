#!/usr/bin/env python3

import sys, random
from ipaddress import IPv4Address, IPv6Address, ip_network
from random import getrandbits, randrange


LOG_SIZE = 1000

if len(sys.argv) > 1:
    LOG_SIZE = int(sys.argv[1])

def rand_v4():
    while True:
        ip = IPv4Address(random.getrandbits(32))
        if ip.is_global: return ip


def rand_v6():
    if False: # subnets and getrandbits(1):
        network = random.sample(subnets, 1)[0]
        if network.prefixlen < 127:
            subnet_addr = IPv6Address(network.network_address + getrandbits(network.max_prefixlen - network.prefixlen))
            subnet_mask = randrange(network.prefixlen + 1, 128)
            return IPv6Network((subnet_addr, subnet_mask), strict=False)
    #            return IPv6Address(network.network_address + getrandbits(network.max_prefixlen - network.prefixlen))
    return IPv6Address(2**125 + getrandbits(125))


def rand_u64():
    while True:
        i = int(random.lognormvariate(20, 10))
        if i < 2**64: return i

for k in range(LOG_SIZE):
    if randrange(2):
        ip = rand_v4()
    else:
        ip = rand_v6()
    print( "%s %s" % (ip, rand_u64()) )