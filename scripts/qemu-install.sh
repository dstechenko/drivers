#!/bin/bash

curl -O http://ftp.us.debian.org/debian/dists/stable/main/installer-arm64/current/images/cdrom/initrd.gz
curl -O http://ftp.us.debian.org/debian/dists/stable/main/installer-arm64/current/images/cdrom/vmlinuz
curl -O -L https://cdimage.debian.org/debian-cd/current/arm64/iso-dvd/debian-12.8.0-arm64-DVD-1.iso

mkdir -p imgs
qemu-img create -f qcow2 qemu/debian-arm.sda.qcow2 32G

qemu-system-aarch64 \
  -accel hvf \
  -m 4G \
  -machine type=virt \
  -cpu max \
  -m 8g \
  -smp 4 \
  -initrd "initrd.gz" \
  -kernel "vmlinuz" \
  -append "console=ttyAMA0" \
  -drive file=debian-12.8.0-arm64-DVD-1.iso,id=cdrom,if=none,media=cdrom \
    -device virtio-scsi-device \
    -device scsi-cd,drive=cdrom \
  -drive file="qemu/debian-arm.sda.qcow2",id=hd,if=none,media=disk \
    -device virtio-scsi-device \
    -device scsi-hd,drive=hd \
  -netdev user,id=net0,hostfwd=tcp::5555-:22 \
    -device virtio-net-device,netdev=net0 \
  -nographic

# chroot /target sh -c "sed -i '/deb cdrom/d' /etc/apt/sources.list"
# chroot /target sh -c "echo 'PermitRootLogin yes' > /etc/ssh/sshd_config.d/root_ssh.conf"
# chroot /target sh -c "mkdir -p /var/run/sshd && /sbin/sshd -D"
# chroot /target sh -c "rm /etc/ssh/sshd_config.d/root_ssh.conf"

scp -P 5555 root@localhost:/boot/vmlinuz qemu/vmlinuz-from-guest
scp -P 5555 root@localhost:/boot/initrd.img qemu/initrd.img-from-guest

rm -rf debian-12.8.0-arm64-DVD-1.iso vmlinuz initrd.gz