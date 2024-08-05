#!/bin/bash

mkdir -p media/data/

if [ "2" != "0" ]; then
    # Témoin (pin thread)
    # READ
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o read -i 50 -t 0 -b 0 -p 0 -r -f testfile >> media/data/read_LL_0
        sudo ./numabench -o read -i 50 -t 0 -b 1 -p 0 -r -f testfile >> media/data/read_LD_0
        sudo ./numabench -o read -i 50 -t 0 -b 0 -p 1 -r -f testfile >> media/data/read_DL_0
        sudo ./numabench -o read -i 50 -t 0 -b 1 -p 1 -r -f testfile >> media/data/read_DD_0
    done
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o read -i 50 -t 1 -b 1 -p 1 -r -f testfile >> media/data/read_LL_0
        sudo ./numabench -o read -i 50 -t 1 -b 0 -p 1 -r -f testfile >> media/data/read_LD_0
        sudo ./numabench -o read -i 50 -t 1 -b 1 -p 0 -r -f testfile >> media/data/read_DL_0
        sudo ./numabench -o read -i 50 -t 1 -b 0 -p 0 -r -f testfile >> media/data/read_DD_0
    done
    
    # WRITE
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o write -i 50 -t 0 -b 0 -p 0 -r -f testfile >> media/data/write_LL_0
        sudo ./numabench -o write -i 50 -t 0 -b 1 -p 0 -r -f testfile >> media/data/write_LD_0
        sudo ./numabench -o write -i 50 -t 0 -b 0 -p 1 -r -f testfile >> media/data/write_DL_0
        sudo ./numabench -o write -i 50 -t 0 -b 1 -p 1 -r -f testfile >> media/data/write_DD_0
    done
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o write -i 50 -t 1 -b 1 -p 1 -r -f testfile >> media/data/write_LL_0
        sudo ./numabench -o write -i 50 -t 1 -b 0 -p 1 -r -f testfile >> media/data/write_LD_0
        sudo ./numabench -o write -i 50 -t 1 -b 1 -p 0 -r -f testfile >> media/data/write_DL_0
        sudo ./numabench -o write -i 50 -t 1 -b 0 -p 0 -r -f testfile >> media/data/write_DD_0
    done

else
    # READ
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o read -m thread -i 50 -t 0 -b 0 -p 0 -r -f testfile >> media/data/read_LL_3
        sudo ./numabench -o read -m thread -i 50 -t 0 -b 1 -p 0 -r -f testfile >> media/data/read_LD_3
        sudo ./numabench -o read -m thread -i 50 -t 0 -b 0 -p 1 -r -f testfile >> media/data/read_DL_3
        sudo ./numabench -o read -m thread -i 50 -t 0 -b 1 -p 1 -r -f testfile >> media/data/read_DD_3
    done
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o read -m thread -i 50 -t 1 -b 1 -p 1 -r -f testfile >> media/data/read_LL_3
        sudo ./numabench -o read -m thread -i 50 -t 1 -b 0 -p 1 -r -f testfile >> media/data/read_LD_3
        sudo ./numabench -o read -m thread -i 50 -t 1 -b 1 -p 0 -r -f testfile >> media/data/read_DL_3
        sudo ./numabench -o read -m thread -i 50 -t 1 -b 0 -p 0 -r -f testfile >> media/data/read_DD_3
    done
    # WRITE
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o write -m thread -i 50 -t 0 -b 0 -p 0 -r -f testfile >> media/data/write_LL_3
        sudo ./numabench -o write -m thread -i 50 -t 0 -b 1 -p 0 -r -f testfile >> media/data/write_LD_3
        sudo ./numabench -o write -m thread -i 50 -t 0 -b 0 -p 1 -r -f testfile >> media/data/write_DL_3
        sudo ./numabench -o write -m thread -i 50 -t 1 -b 0 -p 0 -r -f testfile >> media/data/write_DD_3
    done
    for ((i=1; i<=10; i++)); do
        sudo ./numabench -o write -m thread -i 50 -t 1 -b 1 -p 1 -r -f testfile >> media/data/write_LL_3
        sudo ./numabench -o write -m thread -i 50 -t 1 -b 0 -p 1 -r -f testfile >> media/data/write_LD_3
        sudo ./numabench -o write -m thread -i 50 -t 1 -b 1 -p 0 -r -f testfile >> media/data/write_DL_3
        sudo ./numabench -o write -m thread -i 50 -t 0 -b 1 -p 1 -r -f testfile >> media/data/write_DD_3
    done
fi