from scapy.all import Ether, IP, ICMP, sendp, get_if_hwaddr, UDP, ARP,srp

iface = "ens4"

target_mac = "52:54:00:52:c3:eb"
source_mac = get_if_hwaddr(iface)

target_ip = "192.168.122.120"
# Build the ARP request
arp_req = ARP(pdst=target_ip)
ether = Ether(dst="ff:ff:ff:ff:ff:ff")  # broadcast Ethernet frame

# Combine into one packet
packet = ether / arp_req

# Send and receive responses
ans, _ = srp(packet, iface=iface, timeout=2, verbose=False)

print(ans)
# Process response
for sent, received in ans:
    print(f"IP: {received.psrc}, MAC: {received.hwsrc}")

pkt = Ether(src=source_mac, dst=target_mac) / \
      IP(src="192.168.1.10", dst="192.168.1.20") / \
      UDP(sport=1234, dport=80) / \
      b"Hello via custom MAC!"

sendp(pkt, iface=iface, verbose=True,count=1)