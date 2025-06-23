#include <stdio.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_errno.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_pcapng.h>

struct rte_mbuf* icmp_echo_request_process_packet(struct rte_icmp_hdr* icmp_hdr, rte_be32_t bind_ip, struct rte_mbuf* mbuf, struct rte_ether_addr bind_mac);
