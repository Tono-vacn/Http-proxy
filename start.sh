#!/bin/bash
make clean
make
echo 'Proxy starts running...'
./proxy &
while true ; do continue ; done