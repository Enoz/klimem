##!/bin/sh

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi 

#Clean old artifacts
rmmod klimem
rm /dev/klimem_dev
make clean

# Install

if command -v bear
then
    bear -- make
else
    make
fi
insmod klimem.ko

# Print magic number output
dmesg | grep klimem | tail -1

#Get Magic num
echo "Enter Magic Number:"
read magicnum

mknod /dev/klimem_dev c $magicnum 0
chmod 666 /dev/klimem_dev 

