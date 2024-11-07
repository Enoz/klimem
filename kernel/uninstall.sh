##!/bin/sh

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi 

rmmod klimem
rm /dev/klimem_dev
make clean
