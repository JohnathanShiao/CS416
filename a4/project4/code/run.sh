make clean
make
rm DISKFILE
fusermount -u /tmp/ds1576/mountdir
./tfs -f -s -d /tmp/ds1576/mountdir
