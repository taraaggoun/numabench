mkdir -p media

if [ "$1" != "0" ]
then
    # Témoin (pin thread)
    # READ
    for ((i=1; i<=10; i++))
    do
        # LL
        sudo ./numabench -o read -i 100 -t 0 -b 0 -p 0 -r -f testfile >> media/read_LL_0
        sudo ./numabench -o read -i 100 -t 1 -b 1 -p 1 -r -f testfile >> media/read_LL_0
        # LD
        sudo ./numabench -o read -i 100 -t 0 -b 1 -p 0 -r -f testfile >> media/read_LD_0
        sudo ./numabench -o read -i 100 -t 1 -b 0 -p 1 -r -f testfile >> media/read_LD_0
        # DL
        sudo ./numabench -o read -i 100 -t 0 -b 0 -p 1 -r -f testfile >> media/read_DL_0
        sudo ./numabench -o read -i 100 -t 1 -b 1 -p 0 -r -f testfile >> media/read_DL_0
        # DD
        sudo ./numabench -o read -i 100 -t 0 -b 1 -p 1 -r -f testfile >> media/read_DD_0
        sudo ./numabench -o read -i 100 -t 1 -b 0 -p 0 -r -f testfile >> media/read_DD_0
    done
    # WRITE
    for ((i=1; i<=10; i++))
    do
        # LL
        sudo ./numabench -o write -i 100 -t 0 -b 0 -p 0 -r -f testfile >> media/write_LL_0
        sudo ./numabench -o write -i 100 -t 1 -b 1 -p 1 -r -f testfile >> media/write_LL_0
        # LD
        sudo ./numabench -o write -i 100 -t 0 -b 1 -p 0 -r -f testfile >> media/write_LD_0
        sudo ./numabench -o write -i 100 -t 1 -b 0 -p 1 -r -f testfile >> media/write_LD_0
        # DL
        sudo ./numabench -o write -i 100 -t 0 -b 0 -p 1 -r -f testfile >> media/write_DL_0
        sudo ./numabench -o write -i 100 -t 1 -b 1 -p 0 -r -f testfile >> media/write_DL_0
        # DD
        sudo ./numabench -o write -i 100 -t 0 -b 1 -p 1 -r -f testfile >> media/write_DD_0
        sudo ./numabench -o write -i 100 -t 1 -b 0 -p 0 -r -f testfile >> media/write_DD_0
    done
else
    # READ
    for ((i=1; i<=10; i++))
    do
        # LL
        sudo ./numabench -o read -m thread -i 100 -t 0 -b 0 -p 0 -r -f testfile >> media/read_LL_3
        sudo ./numabench -o read -m thread -i 100 -t 1 -b 1 -p 1 -r -f testfile >> media/read_LL_3
        # LD
        sudo ./numabench -o read -m thread -i 100 -t 0 -b 1 -p 0 -r -f testfile >> media/read_LD_3
        sudo ./numabench -o read -m thread -i 100 -t 1 -b 0 -p 1 -r -f testfile >> media/read_LD_3
        # DL
        sudo ./numabench -o read -m thread -i 100 -t 0 -b 0 -p 1 -r -f testfile >> media/read_DL_3
        sudo ./numabench -o read -m thread -i 100 -t 1 -b 1 -p 0 -r -f testfile >> media/read_DL_3
        # DD
        sudo ./numabench -o read -m thread -i 100 -t 0 -b 1 -p 1 -r -f testfile >> media/read_DD_3
        sudo ./numabench -o read -m thread -i 100 -t 1 -b 0 -p 0 -r -f testfile >> media/read_DD_3
    done
    # WRITE
    for ((i=1; i<=10; i++))
    do
        # LL
        sudo ./numabench -o write -m thread -i 100 -t 0 -b 0 -p 0 -r -f testfile >> media/write_LL_3
        sudo ./numabench -o write -m thread -i 100 -t 1 -b 1 -p 1 -r -f testfile >> media/write_LL_3
        # LD
        sudo ./numabench -o write -m thread -i 100 -t 0 -b 1 -p 0 -r -f testfile >> media/write_LD_3
        sudo ./numabench -o write -m thread -i 100 -t 1 -b 0 -p 1 -r -f testfile >> media/write_LD_3
        # DL
        sudo ./numabench -o write -m thread -i 100 -t 0 -b 0 -p 1 -r -f testfile >> media/write_DL_3
        sudo ./numabench -o write -m thread -i 100 -t 1 -b 1 -p 0 -r -f testfile >> media/write_DL_3
        # DD
        sudo ./numabench -o write -m thread -i 100 -t 1 -b 0 -p 0 -r -f testfile >> media/write_DD_3
        sudo ./numabench -o write -m thread -i 100 -t 0 -b 1 -p 1 -r -f testfile >> media/write_DD_3
fi