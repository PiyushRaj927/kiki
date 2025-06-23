#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_errno.h>
#include <rte_mbuf.h>
#include <rte_ethdev.h>
#include <rte_pcapng.h>

#include "arp.h"
#include "icmp.h"

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define BURST_SIZE 32

static inline int
port_init(uint16_t port, struct rte_mempool* mbuf_pool)
{
    struct rte_eth_conf port_conf;
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = RX_RING_SIZE;
    uint16_t nb_txd = TX_RING_SIZE;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_rxconf rxconf;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    memset(&port_conf, 0, sizeof(struct rte_eth_conf));

    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0) {
        printf("Error during getting device (port %u) info: %s\n",
            port, strerror(-retval));

        return retval;
    }

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |=
        RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;


    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0)
        return retval;

    rxconf = dev_info.default_rxconf;

    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
            rte_eth_dev_socket_id(port), &rxconf, mbuf_pool);
        if (retval < 0)
            return retval;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
            rte_eth_dev_socket_id(port), &txconf);
        if (retval < 0)
            return retval;
    }

    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;


    struct rte_ether_addr addr;

    retval = rte_eth_macaddr_get(port, &addr);
    if (retval < 0) {
        printf("Failed to get MAC address on port %u: %s\n",
            port, rte_strerror(-retval));
        return retval;
    }
    printf("Port %u MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8
        " %02"PRIx8" %02"PRIx8" %02"PRIx8"\n",
        (unsigned)port,
        RTE_ETHER_ADDR_BYTES(&addr));

    // retval = rte_eth_promiscuous_enable(port);
    // if (retval != 0)
    //     return retval;
    return 0;
}

int main(int argc, char** argv)
{
    rte_be32_t bind_ip = rte_cpu_to_be_32(RTE_IPV4(192, 168, 122, 120));
    int ret;
    ret = rte_eal_init(argc, argv);
    rte_devargs_dump(stdout);
    if (ret < 0)
        rte_panic("Cannot init EAL: %s\n", rte_strerror(rte_errno));
    argc -= ret;
    argv += ret;
    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 1)
        rte_exit(EXIT_FAILURE, "no ports found\n");

    struct rte_mempool* mbuf_pool;
    uint64_t NUM_MBUFS = 8191; //todo: follow 2^q-1 for performance
    uint16_t  MBUF_CACHE_SIZE = 250;
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
        NUM_MBUFS * nb_ports, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
    unsigned int portid;

    RTE_ETH_FOREACH_DEV(portid) {
        printf("\nPortID: %d\n", portid);
        if (port_init(portid, mbuf_pool) != 0) {
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu16"\n",
                portid);
        }
    }

    if (rte_lcore_count() > 1)
        printf("\nWARNING: Too much enabled lcores - "
            "App uses only 1 lcore\n");


    struct rte_ether_addr bind_mac;


    while (1) {
        RTE_ETH_FOREACH_DEV(portid) {
            struct rte_mbuf* mbufs[BURST_SIZE];
            const uint16_t nb_rx = rte_eth_rx_burst(portid, 0,
                mbufs, BURST_SIZE);

            // if (nb_rx > 0) {
            //     printf("Received %u packets on port %u\n", nb_rx, portid);
            // }
            struct rte_ether_addr bind_mac;
            int retval;
            retval = rte_eth_macaddr_get(portid, &bind_mac);
            if (retval < 0) {
                printf("Failed to get MAC address on port %u: %s\n",
                    portid, rte_strerror(-retval));
                return retval;
            }
            for (int i = 0; i < nb_rx;i++) {
                struct rte_ether_hdr* ehdr = rte_pktmbuf_mtod(mbufs[i], struct rte_ether_hdr*);
                // if (!rte_is_same_ether_addr(&ehdr->dst_addr, &addr)) {
                //     // printf("unknown mac addr MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n",RTE_ETHER_ADDR_BYTES(&ehdr->dst_addr));
                //     continue;
                // }
                // else {
                //     printf("packet  received from MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n", RTE_ETHER_ADDR_BYTES(&ehdr->src_addr));
                // }
                rte_be16_t packet_ether_type = rte_be_to_cpu_16(ehdr->ether_type);;
                if (packet_ether_type == RTE_ETHER_TYPE_IPV4) {
                    struct rte_ipv4_hdr* iphdr = rte_pktmbuf_mtod_offset(mbufs[i], struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr));
                    printf("ipv4 protocol 0x%x\n", iphdr->next_proto_id);
                    if (iphdr->next_proto_id == IPPROTO_ICMP) {

                        printf("ICMP packet received\n");
                        struct rte_icmp_hdr* icmp_hdr = rte_pktmbuf_mtod_offset(mbufs[i], struct rte_icmp_hdr*, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
                        if (icmp_hdr->icmp_type == RTE_ICMP_TYPE_ECHO_REQUEST) {
                            printf("echo requested\n");
                            struct rte_mbuf* icmp_buf = icmp_echo_request_process_packet(icmp_hdr, bind_ip, mbufs[i], bind_mac);
                            rte_eth_tx_burst(portid, 0, &icmp_buf, 1);
                            printf("ICMP reply packet send\n");
                            rte_pktmbuf_free(icmp_buf);
                        };
                    };
                }
                else if (packet_ether_type == RTE_ETHER_TYPE_ARP) {
                    printf("ARP packet received\n");
                    printf("bind ip 0x%x\n", bind_ip);
                    struct rte_arp_hdr* arp_hdr = rte_pktmbuf_mtod_offset(mbufs[i], struct rte_arp_hdr*, sizeof(struct rte_ether_hdr));
                    printf("target ip 0x%x\n", arp_hdr->arp_data.arp_tip);
                    if (likely(arp_hdr->arp_data.arp_tip == bind_ip)) {
                        printf("ARP RECEIVED from MAC: %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8" %02"PRIx8"\n", RTE_ETHER_ADDR_BYTES(&arp_hdr->arp_data.arp_sha));

                        struct rte_mbuf* arp_buf = arr_process_packet(arp_hdr, bind_ip, mbuf_pool, bind_mac);

                        rte_eth_tx_burst(portid, 0, &arp_buf, 1);
                        printf("ARP packet send\n");
                        rte_pktmbuf_free(arp_buf);
                    }
                }
                else if (packet_ether_type == RTE_ETHER_TYPE_IPV6) {
                    printf("IPv6 packet received\n");
                }
                else {
                    // printf("Unknown EtherType: 0x%x\n", packet_ether_type);
                }

                rte_pktmbuf_free(mbufs[i]);

            };


        }


    }
    /* clean up the EAL */
    rte_eal_cleanup();

    return 0;
}
