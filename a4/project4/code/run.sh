make clean
make
rm DISKFILE
fusermount -u /tmp/ds1576/mountdir
./tfs -s -d /tmp/ds1576/mountdir
