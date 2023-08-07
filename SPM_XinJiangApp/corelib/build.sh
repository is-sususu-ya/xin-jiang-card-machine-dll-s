make BOARD_TYPE=TSP86 1>a.txt  2>a.txt
cat a.txt | grep error
rm a.txt
cp -vf pay /nfsroot/
