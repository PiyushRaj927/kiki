#include <stdio.h>
#include <stdlib.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_errno.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_pcapng.h>


struct rte_mbuf* arr_process_packet(struct rte_arp_hdr* arp_hdr, rte_be32_t bind_ip, struct rte_mempool* mbuf_pool, struct rte_ether_addr bind_mac) {


    const unsigned total_length = sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr);

    struct rte_mbuf* mbuf = rte_pktmbuf_alloc(mbuf_pool);
    if (!mbuf) {
        rte_exit(EXIT_FAILURE, "arp: rte_pktmbuf_alloc\n");
    }

    mbuf->pkt_len = total_length;
    mbuf->data_len = total_length;
    // eth
    uint8_t* pkt_data = rte_pktmbuf_mtod(mbuf, uint8_t*);
    struct rte_ether_hdr* eth = (struct rte_ether_hdr*)pkt_data;
    rte_memcpy(eth->src_addr.addr_bytes, bind_mac.addr_bytes, RTE_ETHER_ADDR_LEN);
    rte_memcpy(eth->dst_addr.addr_bytes, arp_hdr->arp_data.arp_sha.addr_bytes, RTE_ETHER_ADDR_LEN);
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP);

    // 2 arp 
    struct rte_arp_hdr* arp = (struct rte_arp_hdr*)(eth + 1);
    arp->arp_hardware = rte_cpu_to_be_16(1);
    arp->arp_protocol = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    arp->arp_hlen = RTE_ETHER_ADDR_LEN;
    arp->arp_plen = sizeof(uint32_t);
    arp->arp_opcode = rte_cpu_to_be_16(2);
    rte_memcpy(arp->arp_data.arp_sha.addr_bytes, bind_mac.addr_bytes, RTE_ETHER_ADDR_LEN);
    rte_memcpy(arp->arp_data.arp_tha.addr_bytes, arp_hdr->arp_data.arp_sha.addr_bytes, RTE_ETHER_ADDR_LEN);

    arp->arp_data.arp_sip = bind_ip;
    arp->arp_data.arp_tip = arp_hdr->arp_data.arp_sip;
    printf("[ARP REPLY] %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X\n", RTE_ETHER_ADDR_BYTES(&eth->src_addr), RTE_ETHER_ADDR_BYTES(&eth->src_addr));
    return mbuf;


}