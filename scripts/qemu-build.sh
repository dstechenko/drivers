#!/bin/bash

scp -P 5555 -r modules/usb-ipc root@localhost:/home/qemu
ssh -p 5555 root@localhost -f "cd /home/qemu/usb-ipc; make; make install"