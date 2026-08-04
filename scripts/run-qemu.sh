#! /bin/bash

# qemu-system-x86_64 \
/home/isa_codex/src/buildroot/output/build/host-qemu-10.2.0/build/qemu-system-x86_64 \
    -M q35 \
    -m 512M \
    -kernel ~/workspace/virtual_silicon_validation_project/output/images/bzImage \
    -append "console=ttyS0 root=/dev/vda rw panic=-1" \
    -drive file=~/workspace/virtual_silicon_validation_project/output/images/rootfs.ext4,format=raw,if=virtio \
	-device simple-pci \
	-serial stdio \
    -display none \
    -no-reboot
