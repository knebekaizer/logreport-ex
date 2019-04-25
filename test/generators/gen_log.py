#!/usr/bin/env python3

import random

LEN_LOG = 1000

import ipaddress
from ipaddress import IPv4Address, IPv6Address

def rand_v4():
    while True:
        ip = IPv4Address(random.getrandbits(32))
        if ip.is_global: return ip


for k in range(1):
    ip = IPv6Address(random.getrandbits(128))
    #   if ip.is_reserved: print(ip, "reserved")
    if not ip.is_global:
        print(ip, "not global")

def rand_u64():
    while True:
        i = int(random.lognormvariate(20, 10))
        if i < 2**64: return i

for k in range(LEN_LOG):
    ip = rand_v4()
    print( "%s %s" % (ip, rand_u64()) )