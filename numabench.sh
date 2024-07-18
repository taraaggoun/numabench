if [ "$1" != "2" ]
then
    echo "0" > /proc/sys/kernel/numa_balancing
else
    echo "1" > /proc/sys/kernel/numa_balancing
fi

mkdir -p media

dd if=/dev/urandom of=media/file bs=1G count=1

for ((i=1; i<=10; i++))
do
    # LL
    ./numabench -o read -m thread -i 1000 -t 0 -b 0 -p 0 -f testfile >> media/LL_$1
    # LD
    ./numabench -o read -m thread -i 1000 -t 0 -b 1 -p 0 -f testfile >> media/LD_$1
    # DL
    ./numabench -o read -m thread -i 1000 -t 0 -b 0 -p 1 -f testfile >> media/DL_$1
    # DD
    ./numabench -o read -m thread -i 1000 -t 0 -b 1 -p 1 -f testfile >> media/DD_$1
done