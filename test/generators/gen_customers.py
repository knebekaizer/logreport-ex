#!/usr/bin/env python

import random, sys

import ipaddress
from ipaddress import IPv4Address, IPv6Address, ip_network

N_CUSTOMERS = 100000
N_SUBNETS = 2 * N_CUSTOMERS

# generate customers set

words = open('nouns').read().split()
customers_set = set();

# Create set then convert to list, to have a sequence with no dups
for k in range(N_CUSTOMERS):
    n = random.randrange(1, 3)
    w = "-".join([random.choice(words) for i in range(n)])
    if n == 1:
        w = w + "." + random.choice(["com", "info", "org", "tv"])
    while n > 1 and w in customers_set:
        w = w + "_" + str(random.randrange(0, 65536))
    customers_set.add(w)

customers = list(customers_set)
# print(customers)

def rand_v4():
    while True:
        ip = IPv4Address(random.getrandbits(32))
        if ip.is_global:
            return

# use set to avoid duplicates
subnets = set()

def rand_network():
    while True:
        a = random.getrandbits(32)
        m = random.randrange(8, 33)     # ???
        ip = ip_network((a, m), strict=False)
        if ip.is_global and ip not in subnets:
            subnets.add(ip)
            return ip

for k in range(N_SUBNETS):
    c = random.choice(customers)
    ip = rand_network()
    print("%s %s" % (c, ip))

sys.exit(0)

