#!/bin/bash
set -x
sudo dpdk-hugepages.py -p 2M --setup 2G
sudo modprobe uio_pci_generic
sudo dpdk-devbind.py --bind=uio_pci_generic ens5            