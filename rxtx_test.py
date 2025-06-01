from scapy.all import Ether, IP, ICMP, sendp

iface = "ens4"

dst_mac = "52:54:00:86:45:b6"

packet = Ether(dst=dst_mac) / IP(dst="1.1.1.1") / ICMP()
sendp(packet, iface=iface, count=1000)
