#!/bin/bash

check_root() {
    if [ "$(id -u)" -ne 0 ]; then
        echo "Ce script doit être exécuté en tant que root." >&2
        exit 1
    fi
}
check_root

check_and_install_package() {
    PKG_NAME=$1
    if ! dpkg -s $PKG_NAME >/dev/null 2>&1; then
        echo "Package $PKG_NAME is not installed. Installing..."
        sudo apt-get install -y $PKG_NAME
    else
        echo "Package $PKG_NAME is already installed."
    fi
}

LIBRARIES=("libnuma-dev" "gcc" "make" "wget")

for lib in "${LIBRARIES[@]}"; do
    check_and_install_package "$lib"
done

cd compression

# KERNEL_ARCHIVE="linux-6.6.21.tar.xz"
# KERNEL_URL="https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.21.tar.xz"

# if [ ! -f $KERNEL_ARCHIVE ]; then
#     echo "Downloading Linux kernel version..."
#     wget $KERNEL_URL
# else
#     echo "Linux kernel archive ${KERNEL_ARCHIVE} already downloaded."
# fi

# if [ ! -d "${KERNEL_ARCHIVE}" ]; then
#     echo "Decompressing Linux kernel archive..."
#     tar -xvf $KERNEL_ARCHIVE
# else
#     echo "Linux kernel already decompressed."
# fi

dd if=/dev/zero of=testfile bs=1M count=5120

echo "Compiling the C code..."
gcc -o compression compression.c -lnuma
gcc -o run_compression run_compression.c -lnuma


if [ $? -eq 0 ]; then
    echo "Compilation successful. Executing the program..."
else
    echo "Compilation failed."
    exit 1
fi

./run_compression 0

echo "Cleaning up..."
rm -rf "compression/linux* compression/compression"
cd ..

echo "Done."