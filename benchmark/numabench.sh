if [ "$1" != "2" ]
then
    echo "0" > /proc/sys/kernel/numa_balancing
else
    echo "1" > /proc/sys/kernel/numa_balancing
fi

mkdir -p media

for ((i=1; i<=10; i++))
do
    # LL
    ./numabench -o write -m thread -i 1000 -t 0 -b 0 -p 0 -r -f testfile >> media/write_LL_$1
    # LD
    ./numabench -o write -m thread -i 1000 -t 0 -b 1 -p 0 -r -f testfile >> media/write_LD_$1
    # DL
    ./numabench -o write -m thread -i 1000 -t 0 -b 0 -p 1 -r -f testfile >> media/write_DL_$1
    # DD
    ./numabench -o write -m thread -i 1000 -t 0 -b 1 -p 1 -r -f testfile >> media/write_DD_$1
done

for ((i=1; i<=10; i++))
do
    # LL
    ./numabench -o read -m thread -i 1000 -t 0 -b 0 -p 0 -r -f testfile >> media/read_LL_$1
    # LD
    ./numabench -o read -m thread -i 1000 -t 0 -b 1 -p 0 -r -f testfile >> media/read_LD_$1
    # DL
    ./numabench -o read -m thread -i 1000 -t 0 -b 0 -p 1 -r -f testfile >> media/read_DL_$1
    # DD
    ./numabench -o read -m thread -i 1000 -t 0 -b 1 -p 1 -r -f testfile >> media/read_DD_$1
done