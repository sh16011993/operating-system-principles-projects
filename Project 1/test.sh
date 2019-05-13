#!/bin/bash
cd kernel_module
make
sudo make install
cd ..
sudo insmod kernel_module/processor_container.ko
sudo chmod 777 /dev/pcontainer
./benchmark/benchmark "$@"
sudo rmmod processor_container
