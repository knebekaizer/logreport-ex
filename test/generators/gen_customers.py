#!/usr/bin/env python

import random

N_CUSTOMERS = 20
N_SUBNETS = 2 * N_CUSTOMERS

# generate customers set

words = open('nouns').read().split()
customers_set = set();

for k in range(N_CUSTOMERS):
    n = random.randrange(1, 3)
    w = "-".join([random.choice(words) for i in range(n)])
    if n == 1:
        w = w + "." + random.choice(["com", "info", "org", "tv"])
    while n > 1 and w in customers_set:
        w = w + "_" + str(random.randrange(0, 65536))
    customers_set.add(w)
# print(customers_set)

def random_v4():
    i = random.randrange(1, 256)
    if i == 10: i += 1
    i = (i << 8) + random.randrange(0, 256)
    if i == (192 << 8 + 168): i += 1
    i = (i << 8) + random.randrange(0, 256)
    i = (i << 8) + random.randrange(0, 256)
    return i

def str_v4(ip):
    return ".".join(str((ip >> b) & 255) for b in [24,16,8,0])

MAX_I4 = 0xffffffff
def random_sub():
    bits = random.randrange(16, 33)     # ???
    mask = (MAX_I4 << (32 - bits)) & MAX_I4
    ip = random_v4() & mask
#    print("%s/%d" % (str_v4(ip), bits))
    return (ip, bits)

for k in range(N_SUBNETS):
    random_sub()

import ipaddress
from ipaddress import IPv4Address, IPv6Address
for k in range(1000):
    ip = IPv4Address(random.getrandbits(32))
#   if ip.is_reserved: print(ip, "reserved")
    if not ip.is_global:
        print(ip, "not global")

for k in range(1000):
    ip = IPv6Address(random.getrandbits(128))
    #   if ip.is_reserved: print(ip, "reserved")
    if not ip.is_global:
        print(ip, "not global")
