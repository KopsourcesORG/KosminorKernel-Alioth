#!/bin/sh
#
# Compile script kernel
# Copyright (C) 2024-2025 Rve.

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

CLANG_DIR=""
DEFCONFIG=""
KBUILD_BUILD_USER="$(whoami)"
KBUILD_BUILD_HOST="$(hostname)"
CLEAN_BUILD="false"

while [ $# -gt 0 ]; do
    case $1 in
        DEFCONFIG=*)
            DEFCONFIG="${1#*=}"
            shift
            ;;
        CLANG_DIR=*)
            CLANG_DIR="${1#*=}"
            shift
            ;;
        KBUILD_BUILD_USER=*)
            KBUILD_BUILD_USER="${1#*=}"
            shift
            ;;
        KBUILD_BUILD_HOST=*)
            KBUILD_BUILD_HOST="${1#*=}"
            shift
            ;;
        CLEAN_BUILD=*)
            CLEAN_BUILD="${1#*=}"
            shift
            ;;
        *)
            echo ""
            echo "${RED}Unknown parameter: $1${NC}"
            echo "Usage: $0 DEFCONFIG=<defconfig_name> CLANG_DIR=<clang_path> [KBUILD_BUILD_USER=<user>] [KBUILD_BUILD_HOST=<host>]"
            echo "Example: $0 DEFCONFIG=your_defconfig CLANG_DIR=/home/rve/clang"
            echo "Example: $0 DEFCONFIG=your_defconfig CLANG_DIR=/home/rve/clang KBUILD_BUILD_USER=MyUser KBUILD_BUILD_HOST=MyHost"
            exit 1
            ;;
    esac
done

if [ -z "$DEFCONFIG" ]; then
    echo ""
    echo "${RED}Error: DEFCONFIG parameter is required${NC}"
    echo "Usage: $0 DEFCONFIG=<defconfig_name> CLANG_DIR=<clang_path>"
    echo "Example: $0 DEFCONFIG=rve_defconfig CLANG_DIR=/home/rve/clang"
    exit 1
fi

if [ -z "$CLANG_DIR" ]; then
    echo ""
    echo "${RED}Error: CLANG_DIR parameter is required${NC}"
    echo "Usage: $0 DEFCONFIG=<defconfig_name> CLANG_DIR=<clang_path>"
    echo "Example: $0 DEFCONFIG=rve_defconfig CLANG_DIR=/home/rve/clang"
    exit 1
fi

if [ ! -d "$CLANG_DIR" ]; then
    echo ""
    echo "${RED}Error: CLANG_DIR path does not exist: $CLANG_DIR${NC}"
    exit 1
fi

echo ""
echo "${GREEN}Using DEFCONFIG: $DEFCONFIG${NC}"
echo "${GREEN}Using CLANG_DIR: $CLANG_DIR${NC}"
echo "${GREEN}Using KBUILD_BUILD_USER: $KBUILD_BUILD_USER${NC}"
echo "${GREEN}Using KBUILD_BUILD_HOST: $KBUILD_BUILD_HOST${NC}"

if [ "$CLEAN_BUILD" = "true" ]; then
    if [ -d "out" ]; then
        echo ""
        echo "${GREEN}Clean build requested - removing out directory...${NC}"
        rm -rf out
    else
        echo "${GREEN}Clean build requested but out directory doesn't exist${NC}"
    fi
fi

if [ ! -d "out" ]; then
    echo ""
    echo "${GREEN}Creating out directory...${NC}"
    mkdir -p out
else
    echo ""
    echo "${GREEN}Out directory already exists${NC}"
fi

if [ -f "out/compile.log" ]; then
    echo ""
    echo "${GREEN}Removing old compile.log...${NC}"
    rm -f out/compile.log
fi

export KBUILD_BUILD_USER=$KBUILD_BUILD_USER
export KBUILD_BUILD_HOST=$KBUILD_BUILD_HOST
export PATH="$CLANG_DIR/bin:$PATH"

make O=out ARCH=arm64 $DEFCONFIG

compile () {
    make -j$(nproc --all) O=out LLVM=1 LLVM_IAS=1 \
    ARCH=arm64 \
    CC=clang \
    LD=ld.lld \
    AR=llvm-ar \
    AS=llvm-as \
    NM=llvm-nm \
    STRIP=llvm-strip \
    OBJCOPY=llvm-objcopy \
    OBJDUMP=llvm-objdump \
    READELF=llvm-readelf \
    HOSTCC=clang \
    HOSTCXX=clang++ \
    HOSTAR=llvm-ar \
    HOSTLD=ld.lld \
    CROSS_COMPILE=arm64-linux-gnu-
}

compile 2>&1 | tee -a out/compile.log
