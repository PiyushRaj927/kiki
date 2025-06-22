#!/bin/bash
set -x
sudo modprobe uio_pci_generic
sudo dpdk-devbind.py --bind=uio_pci_generic ens5            