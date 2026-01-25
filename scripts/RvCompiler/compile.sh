#!/bin/bash
#
# Compile script kernel
# Copyright (C) 2024-2026 Rve.

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

CLANG_DIR=""
DEFCONFIG=""
KBUILD_BUILD_USER="$(whoami)"
KBUILD_BUILD_HOST="$(hostname)"
CLEAN_BUILD="false"

show_help() {
    echo ""
    echo -e "${GREEN}Usage:${NC} $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -d, --defconfig <name>    Set defconfig name (Required)"
    echo "  -c, --clang-dir <path>    Set path to Clang directory (Required)"
    echo "  -u, --user <name>         Set KBUILD_BUILD_USER (Default: $(whoami))"
    echo "  -H, --host <name>         Set KBUILD_BUILD_HOST (Default: $(hostname))"
    echo "      --clean               Clean build output before compiling"
    echo "  -h, --help                Show this help message"
    echo ""
    echo "Example:"
    echo "  $0 -d rve_defconfig -c toolchain/aosp-clang"
}

while [ $# -gt 0 ]; do
    case $1 in
        -d|--defconfig)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                DEFCONFIG="$2"
                shift 2
            else
                echo -e "${RED}Error: Argument for $1 is missing${NC}" >&2
                exit 1
            fi
            ;;
        -c|--clang-dir)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                CLANG_DIR="$2"
                shift 2
            else
                echo -e "${RED}Error: Argument for $1 is missing${NC}" >&2
                exit 1
            fi
            ;;
        -u|--user)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                KBUILD_BUILD_USER="$2"
                shift 2
            else
                echo -e "${RED}Error: Argument for $1 is missing${NC}" >&2
                exit 1
            fi
            ;;
        -H|--host)
            if [ -n "$2" ] && [ "${2:0:1}" != "-" ]; then
                KBUILD_BUILD_HOST="$2"
                shift 2
            else
                echo -e "${RED}Error: Argument for $1 is missing${NC}" >&2
                exit 1
            fi
            ;;
        --clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

if [ -z "$DEFCONFIG" ]; then
    echo ""
    echo -e "${RED}Error: DEFCONFIG is required${NC}"
    show_help
    exit 1
fi

if [ -z "$CLANG_DIR" ]; then
    echo ""
    echo -e "${RED}Error: CLANG_DIR is required${NC}"
    show_help
    exit 1
fi

if [ ! -d "$CLANG_DIR" ]; then
    echo ""
    echo -e "${RED}Error: CLANG_DIR path does not exist: $CLANG_DIR${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}Using DEFCONFIG: $DEFCONFIG${NC}"
echo -e "${GREEN}Using CLANG_DIR: $CLANG_DIR${NC}"
echo -e "${GREEN}Using KBUILD_BUILD_USER: $KBUILD_BUILD_USER${NC}"
echo -e "${GREEN}Using KBUILD_BUILD_HOST: $KBUILD_BUILD_HOST${NC}"

if [ "$CLEAN_BUILD" = "true" ]; then
    if [ -d "out" ]; then
        echo ""
        echo -e "${GREEN}Clean build requested - removing out directory...${NC}"
        rm -rf out
    else
        echo -e "${GREEN}Clean build requested but out directory doesn't exist${NC}"
    fi
fi

if [ ! -d "out" ]; then
    echo ""
    echo -e "${GREEN}Creating out directory...${NC}"
    mkdir -p out
else
    echo ""
    echo -e "${GREEN}Out directory already exists${NC}"
fi

if [ -f "out/compile.log" ]; then
    echo ""
    echo -e "${GREEN}Removing old compile.log...${NC}"
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
