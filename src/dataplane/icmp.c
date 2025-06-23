#include <stdio.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_errno.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_pcapng.h>

struct rte_mbuf* icmp_echo_request_process_packet(struct rte_icmp_hdr* icmp_hdr, rte_be32_t bind_ip, struct rte_mbuf* mbuf, struct rte_ether_addr bind_mac) {


    uint8_t* pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t*);

    // eth
    struct rte_ether_hdr* eth = (struct rte_ether_hdr*)pkt_data;
    rte_ether_addr_copy(&eth->src_addr, &eth->dst_addr);
    rte_ether_addr_copy(&bind_mac,&eth->src_addr);

    // ip
    struct rte_ipv4_hdr* ip = (struct rte_ipv4_hdr*)(pkt_data + sizeof(struct rte_ether_hdr));
    ip->dst_addr = ip->src_addr;
    ip->src_addr = bind_ip;

    ip->hdr_checksum = 0;
    ip->hdr_checksum = rte_ipv4_cksum(ip);

    // ICMP 
    struct rte_icmp_hdr* icmp = (struct rte_icmp_hdr*)(pkt_data + sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
    icmp->icmp_type = RTE_ICMP_TYPE_ECHO_REPLY;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = rte_raw_cksum(icmp, rte_be_to_cpu_16(ip->total_length) - sizeof(struct rte_ipv4_hdr));

    printf("ICMP reply packet created\n");
    printf("[ICMP REPLY] %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X | %X -> %X | icmp_seq=%u\n", RTE_ETHER_ADDR_BYTES(&eth->src_addr), RTE_ETHER_ADDR_BYTES(&eth->src_addr),
    ip->src_addr, ip->dst_addr, rte_be_to_cpu_16(icmp->icmp_seq_nb));
    return mbuf;

}