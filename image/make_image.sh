#!/bin/bash
IMG="../build/bin/boot.img"
rm -f $IMG
dd if=/dev/zero of=$IMG bs=1M count=35 > /dev/null
fdisk $IMG << EOF > /dev/null
g
n
1
2048

t
1
w
EOF
mkfs.fat $IMG -F 32 --offset=2048 > /dev/null
mcopy -i $IMG@@1M -s ./root/* ::/ > /dev/null
