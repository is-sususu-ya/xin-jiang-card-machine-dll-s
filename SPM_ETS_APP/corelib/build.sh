make BOARD_TYPE=TMP56 1>a.txt  2>a.txt
cat a.txt | grep error
rm a.txt
cp -vf pay /nfsroot/
