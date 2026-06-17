#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-debug}"

CXX="${CXX:-g++}"
CC="${CC:-gcc}"
CXXFLAGS=""
CFLAGS=""
LDFLAGS=""
ALSA_CFLAGS=""
ALSA_LIBS=""
OUTPUT_DIR=""
OUTPUT_NAME=""

SRC_DIR="src"

INCLUDE_FLAGS=(
    -I"$SRC_DIR"
    -I"$SRC_DIR/Source"
    -I"$SRC_DIR/Source/dsp"
)

CPP_SOURCES=(
    Source/main.cpp
    Source/dsp/Serializer.cpp
    Source/dsp/TableBlep.cpp
)

C_SOURCES=(
)

configure_native_pkg_config() {
    ALSA_CFLAGS="$(pkg-config --cflags alsa)"
    ALSA_LIBS="$(pkg-config --libs alsa)"
}

configure_cross_tools() {
    CC="${CXX%g++}gcc"
}

compiler_supports_flag() {
    local compiler="$1"
    local language="$2"
    local flag="$3"
    echo "int main(void) { return 0; }" | "$compiler" "$flag" -x "$language" -c -o /dev/null - >/dev/null 2>&1
}

native_optional_cxx_flag() {
    local flag="$1"
    if compiler_supports_flag "$CXX" c++ "$flag"; then
        printf ' %s' "$flag"
    fi
}

native_optional_c_flag() {
    local flag="$1"
    if compiler_supports_flag "$CC" c "$flag"; then
        printf ' %s' "$flag"
    fi
}

case "$BUILD_TYPE" in
    debug)
        echo "--- Building Debug (native x86_64) ---"
        CXX="clang++"
        CC="clang"
        CXXFLAGS="-std=c++20 -g -Wall -Wextra -O0 -DTARGET_NATIVE_LINUX"
        CFLAGS="-std=c11 -g -Wall -Wextra -O0 -DTARGET_NATIVE_LINUX"
        configure_native_pkg_config
        OUTPUT_DIR="build/debug"
        OUTPUT_NAME="dirtylet"
        ;;

    release)
        echo "--- Building Release (native x86_64) ---"
        CXX="clang++"
        CC="clang"
        CXXFLAGS="-std=c++20 -Wall -Wextra -Ofast$(native_optional_cxx_flag -march=native) -funroll-loops -pthread -DNDEBUG -DTARGET_NATIVE_LINUX"
        CFLAGS="-std=c11 -Wall -Wextra -Ofast$(native_optional_c_flag -march=native) -funroll-loops -pthread -DNDEBUG -DTARGET_NATIVE_LINUX -D_GNU_SOURCE"
        configure_native_pkg_config
        OUTPUT_DIR="build/release"
        OUTPUT_NAME="dirtylet"
        ;;

    release-rk3506)
        echo "--- Building Release (RK3506 32-bit ARM) ---"
        CLANG_TOOLCHAIN_ROOT="/home/hiirofox/rk3506-clang-toolchain/dist/rk3506-clang-portable"
        CXX="$CLANG_TOOLCHAIN_ROOT/bin/arm-rk3506-linux-gnueabihf-clang++"
        CC="$CLANG_TOOLCHAIN_ROOT/bin/arm-rk3506-linux-gnueabihf-clang"
        SYSROOT="$CLANG_TOOLCHAIN_ROOT/sysroot"
        if [ ! -x "$CXX" ]; then
            echo "error: compiler not found: $CXX"
            exit 1
        fi
        if [ ! -x "$CC" ]; then
            echo "error: compiler not found: $CC"
            exit 1
        fi
        if [ ! -d "$SYSROOT" ]; then
            echo "error: sysroot not found: $SYSROOT"
            echo "please build or extract the RK3506 clang portable toolchain first"
            exit 1
        fi
        CXXFLAGS="-std=c++20 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX"
        CFLAGS="-std=c11 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX -D_GNU_SOURCE"
        ALSA_LIBS="-lasound -lm -lpthread -ldl"
        LDFLAGS="-fuse-ld=lld"
        OUTPUT_DIR="build/release-rk3506"
        OUTPUT_NAME="dirtylet-rk3506"
        ;;

    release-rv1106)
        echo "--- Building Release (RV1106 uClibc) ---"
        SDK_ROOT="/home/hiirofox/rv1103-sdk/luckfox-pico"
        CXX="$SDK_ROOT/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++"
        SYSROOT="$SDK_ROOT/sysdrv/source/buildroot/buildroot-2023.02.6/output/host/arm-buildroot-linux-uclibcgnueabihf/sysroot"
        configure_cross_tools
        if [ ! -f "$CXX" ]; then
            echo "error: compiler not found: $CXX"
            exit 1
        fi
        if [ ! -d "$SYSROOT" ]; then
            echo "error: RV1106 sysroot not found: $SYSROOT"
            echo "please build the SDK first"
            exit 1
        fi
        CXXFLAGS="-std=c++20 -Wall -O3 -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -DNDEBUG -DTARGET_NATIVE_LINUX --sysroot=$SYSROOT"
        CFLAGS="-std=c11 -Wall -O3 -mcpu=cortex-a7 -mfloat-abi=hard -mfpu=neon-vfpv4 -DNDEBUG -DTARGET_NATIVE_LINUX --sysroot=$SYSROOT"
        ALSA_LIBS="-lasound -lm -lpthread -ldl"
        OUTPUT_DIR="build/release-rv1106"
        OUTPUT_NAME="dirtylet-rv1106"
        ;;

    release-rk3566)
        echo "--- Building Release (RK3566 64-bit ARM) ---"
        CLANG_TOOLCHAIN_ROOT="/home/hiirofox/rk3566-clang-toolchain/dist/rk3566-clang-portable"
        CXX="$CLANG_TOOLCHAIN_ROOT/bin/arm-rk3566-linux-gnu-clang++"
        CC="$CLANG_TOOLCHAIN_ROOT/bin/arm-rk3566-linux-gnu-clang"
        LD="$CLANG_TOOLCHAIN_ROOT/gcc-toolchain/bin/aarch64-none-linux-gnu-ld"
        SYSROOT="$CLANG_TOOLCHAIN_ROOT/sysroot"
        if [ ! -x "$CXX" ]; then
            echo "error: compiler not found: $CXX"
            exit 1
        fi
        if [ ! -x "$CC" ]; then
            echo "error: compiler not found: $CC"
            exit 1
        fi
        if [ ! -x "$LD" ]; then
            echo "error: linker not found: $LD"
            exit 1
        fi
        if [ ! -d "$SYSROOT" ]; then
            echo "error: sysroot not found: $SYSROOT"
            echo "please build or extract the RK3566 clang portable toolchain first"
            exit 1
        fi
        CXXFLAGS="-std=c++20 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX"
        CFLAGS="-std=c11 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX -D_GNU_SOURCE"
        LDFLAGS="-fuse-ld=lld"
        ALSA_LIBS="-lasound -lm -lpthread -ldl"
        OUTPUT_DIR="build/release-rk3566"
        OUTPUT_NAME="dirtylet-rk3566"
        ;;

    release-t153)
        echo "--- Building Release (T153 32-bit ARM) ---"
        CLANG_TOOLCHAIN_ROOT="/home/hiirofox/t153-clang-toolchain/dist/t153-clang-portable"
        CXX="$CLANG_TOOLCHAIN_ROOT/bin/arm-t153-linux-gnueabihf-clang++"
        CC="$CLANG_TOOLCHAIN_ROOT/bin/arm-t153-linux-gnueabihf-clang"
        LD="$CLANG_TOOLCHAIN_ROOT/gcc-toolchain/bin/arm-linux-gnueabihf-ld"
        SYSROOT="$CLANG_TOOLCHAIN_ROOT/sysroot"
        if [ ! -x "$CXX" ]; then
            echo "error: compiler not found: $CXX"
            exit 1
        fi
        if [ ! -x "$CC" ]; then
            echo "error: compiler not found: $CC"
            exit 1
        fi
        if [ ! -x "$LD" ]; then
            echo "error: linker not found: $LD"
            exit 1
        fi
        if [ ! -d "$SYSROOT" ]; then
            echo "error: sysroot not found: $SYSROOT"
            echo "please build or extract the T153 clang portable toolchain first"
            exit 1
        fi
        CXXFLAGS="-std=c++20 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX"
        CFLAGS="-std=c11 -Wall -Wextra -Ofast -ffast-math -DNDEBUG -DTARGET_NATIVE_LINUX -D_GNU_SOURCE"
        LDFLAGS="--ld-path=$LD"
        ALSA_LIBS="-lasound -lm -lpthread -ldl"
        OUTPUT_DIR="build/release-t153"
        OUTPUT_NAME="dirtylet-t153"
        ;;

    *)
        echo "error: unknown build type: $BUILD_TYPE"
        echo "usage: ./build.sh debug | release | release-rk3506 | release-rk3566 | release-rv1106 | release-t153"
        exit 1
        ;;
esac

OUTPUT="$OUTPUT_DIR/$OUTPUT_NAME"
OBJ_DIR="$OUTPUT_DIR/obj"
mkdir -p "$OBJ_DIR"

OBJECTS=()

compile_cpp() {
    local src="$1"
    local obj="$OBJ_DIR/${src//\//__}.o"
    mkdir -p "$(dirname "$obj")"
    echo "CXX $src"
    "$CXX" $CXXFLAGS $ALSA_CFLAGS "${INCLUDE_FLAGS[@]}" -c "$src" -o "$obj"
    OBJECTS+=("$obj")
}

compile_c() {
    local src="$1"
    local obj="$OBJ_DIR/${src//\//__}.o"
    mkdir -p "$(dirname "$obj")"
    echo "CC  $src"
    "$CC" $CFLAGS $ALSA_CFLAGS "${INCLUDE_FLAGS[@]}" -c "$src" -o "$obj"
    OBJECTS+=("$obj")
}

echo "building $OUTPUT"
echo "CXX: $CXX"
echo "CC:  $CC"

for src in "${CPP_SOURCES[@]}"; do
    compile_cpp "$src"
done

for src in "${C_SOURCES[@]}"; do
    compile_c "$src"
done

echo "LINK $OUTPUT"
"$CXX" $CXXFLAGS $LDFLAGS "${OBJECTS[@]}" $ALSA_LIBS -lm -lpthread -ldl -o "$OUTPUT"

echo "build succeeded: $OUTPUT"
file "$OUTPUT"
