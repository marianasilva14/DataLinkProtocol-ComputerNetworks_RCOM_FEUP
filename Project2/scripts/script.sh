#!/bin/bash
ifconfig eth0 172.16.41.0/24
route add -net 172.16.41.0/24 gw 172.16.41.254
route add default gw 172.16.41.253
route -n
arp -a