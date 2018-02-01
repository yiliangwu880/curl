
#!/bin/sh

make clean &&
make &&
sh stop.sh &&
./UseCurl.bin