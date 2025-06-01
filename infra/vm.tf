terraform {
  required_providers {
    libvirt = {
      source  = "dmacvicar/libvirt"
      version = "0.8.3"
    }
  }
}
provider "libvirt" {
  uri = "qemu:///system"
}

# # A pool for all cluster volumes
# resource "libvirt_pool" "cluster" {
#   name = "cluster"
#   type = "dir"
#   target {
# 	path = "${abspath(path.module)}/storage-pool"
#   }
# }


# Defining VM Volume
resource "libvirt_volume" "base" {
  name   = "base"
  pool   = "default" # List storage pools using virsh pool-list
  source = "${path.module}/debian-12-generic-amd64.qcow2"
  format = "qcow2"
  # depends_on = [ libvirt_pool.cluster ]
}

resource "libvirt_volume" "vm1" {
  name           = "vm1"
  pool           = "default" # List storage pools using virsh pool-list
  base_volume_id = resource.libvirt_volume.base.id
  size           = 32212254720
  # depends_on = [ libvirt_pool.cluster ]
}


# Use CloudInit to add the instance
resource "libvirt_cloudinit_disk" "commoninit" {
  name      = "commoninit.iso"
  user_data = templatefile("${path.module}/cloud_init.cfg", {})
}

# Define KVM domain to create
resource "libvirt_domain" "vm1" {
  name   = "dpdk-vm1"
  memory = "4096"
  vcpu   = 8
  # running = false
  cpu {
    mode = "host-passthrough"
  }
  filesystem {
    source   = abspath("${path.module}/..")
    target   = "DPDK-LB"
    readonly = false
  }
  network_interface {
    network_name = "default"
  }
  # Emulated DPDK test NIC 1
    network_interface {
    network_name = "default" 
  }

  # Emulated DPDK test NIC 2
    network_interface {
    network_name = "default"
  }

  # Emulated DPDK test NIC 3
    network_interface {
    network_name = "default" 
  }

  disk {
    volume_id = libvirt_volume.vm1.id
  }

  cloudinit = libvirt_cloudinit_disk.commoninit.id

  console {
    type        = "pty"
    target_type = "serial"
    target_port = "0"
  }

  # graphics {
  #   type = "vnc"
  #   listen_type = "address"
  #   autoport = true
  # }
  # lifecycle {
  #   replace_triggered_by = [ libvirt_cloudinit_disk.commoninit.user_data ]
  # }
}
output "VM_Name" {
  value = libvirt_domain.vm1.name
}
output "VM_Memory" {
  value = libvirt_domain.vm1.memory
}

output "VM_IP" {
  value = try(libvirt_domain.vm1.network_interface.0.addresses.0, "IP not available yet")
}