#!/bin/bash

../qemu/build/qemu-system-aarch64 \
  -accel hvf \
  -m 4G \
  -machine type=virt \
  -cpu max \
  -m 8g \
  -smp 4 \
  -initrd "qemu/initrd.img-from-guest" \
  -kernel "qemu/vmlinuz-from-guest" \
  -append "console=ttyAMA0 root=/dev/sda2" \
  -drive file="qemu/debian-arm.sda.qcow2",id=hd,if=none,media=disk \
    -device virtio-scsi-device \
    -device scsi-hd,drive=hd \
  -netdev user,id=net0,hostfwd=tcp::5555-:22 \
    -device virtio-net-device,netdev=net0 \
  -usb \
    -device qemu-xhci \
    -device usb-echo \
  -nographic