#!/usr/bin/env python3

import random, sys, string

from random import getrandbits, randrange

import ipaddress
from ipaddress import IPv4Address, IPv6Network, IPv6Address, ip_network


N_CUSTOMERS = 100    # 100000

if len(sys.argv) > 1:
    N_CUSTOMERS = int(sys.argv[1])

N_SUBNETS = 2 * N_CUSTOMERS

# generate customers set

import nouns
words = nouns.words # open('nouns').read().split()
customers_set = set();
subnets4 = set()
subnets6 = set()


#random.seed(1)   # Make the test repeatable

strict = True # Use strict Ids

# Create set then convert to list, to have a sequence with no dups
for k in range(N_CUSTOMERS):
    n = randrange(1, 3)
    if not strict:
        w = "-".join([random.choice(words) for i in range(n)])
        if n == 1:
            w = w + "." + random.choice(["com", "info", "org", "tv"])
        while n > 1 and w in customers_set:
            w = w + "_" + str(randrange(0, 65536))
    else:
        w = "".join([random.choice(words) for i in range(n)]).upper()
        while n > 1 and w in customers_set:
            w = w + ''.join([random.choice(string.ascii_uppercase) for n in range(6)])
    customers_set.add(w)

customers = sorted(list(customers_set))
#print(customers)

# use set to avoid duplicates

def rand_v4():
    a = getrandbits(32)
    m = randrange(8, 33)     # ???
    return ip_network((a, m), strict=False)


def rand_v6():
    if subnets6 and getrandbits(1):
        network = random.sample(subnets6, 1)[0]
        if network.prefixlen < 127:
            subnet_addr = IPv6Address(network.network_address + getrandbits(network.max_prefixlen - network.prefixlen))
            subnet_mask = randrange(network.prefixlen + 1, 128)
            return IPv6Network((subnet_addr, subnet_mask), strict=False)
#            return IPv6Address(network.network_address + getrandbits(network.max_prefixlen - network.prefixlen))
    return ip_network((2**125 + getrandbits(125), randrange(16, 128)), strict=False)

def rand_network():
    while True:
        generator = random.choice([rand_v4, rand_v6])
        ip = generator()
        if ip.version == 4:
            subnets = subnets4
        else:
            subnets = subnets6
        if ip.is_global and ip not in subnets:
            subnets.add(ip)
            return ip

for k in range(N_SUBNETS):
    c = random.choice(customers)
    ip = rand_network()
    print("%s %s" % (ip, c))

sys.exit(0)

