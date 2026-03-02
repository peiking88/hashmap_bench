#!/bin/bash
#
# hashmap_bench One-Click Build Script
# Handles: submodule initialization, autoconf config generation, cmake build
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# ============================================================================
# Step 1: Initialize all submodules
# ============================================================================
init_submodules() {
    log_info "Initializing submodules..."
    
    # Sync submodule URLs from .gitmodules
    git submodule sync --quiet
    
    # List of submodules that may need manual checkout
    local submodules=(
        "external/rhashmap"
        "external/ssmem"
        "external/sspfd"
        "external/opic"
        "external/sparsehash"
        "external/parallel-hashmap"
        "external/abseil-cpp"
        "external/Catch2"
        "external/container"
        "external/folly"
        "external/NanoLog"
        "external/cista"
        "external/clht"
        "external/libcuckoo"
        "external/libfork"
    )
    
    for sm in "${submodules[@]}"; do
        if [ ! -f "$sm/.git" ] || [ -z "$(ls -A "$sm" 2>/dev/null | grep -v '^.git$')" ]; then
            log_info "Fetching submodule: $sm"
            git submodule update --init -- "$sm" 2>/dev/null || {
                # Fallback: manual checkout
                cd "$sm"
                git fetch origin 2>/dev/null || true
                git checkout HEAD 2>/dev/null || true
                cd "$SCRIPT_DIR"
            }
        fi
    done
    
    log_info "Submodules initialized."
}

# ============================================================================
# Step 2: Generate OPIC config.h
# ============================================================================
generate_opic_config() {
    local config_dir="$SCRIPT_DIR/stubs/opic"
    local config_file="$config_dir/config.h"
    
    if [ -f "$config_file" ]; then
        log_info "OPIC config.h already exists."
        return
    fi
    
    log_info "Generating OPIC config.h..."
    mkdir -p "$config_dir"
    
    cat > "$config_file" << 'EOF'
/* config.h for OPIC - generated for CMake build */

#ifndef OPIC_CONFIG_H
#define OPIC_CONFIG_H

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `malloc' function. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the `memcmp' function. */
#define HAVE_MEMCMP 1

/* Define to 1 if you have the `mmap' function. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `pthread' library. */
#define HAVE_PTHREAD 1

/* Most likely not having log4c */
/* #undef HAS_LOG4C */

/* Package name */
#define PACKAGE "opic"

/* Package version */
#define PACKAGE_VERSION "0.8.0"

/* Version string */
#define VERSION "0.8.0"

#endif /* OPIC_CONFIG_H */
EOF
    
    log_info "OPIC config.h generated."
}

# ============================================================================
# Step 3: Generate sparsehash sparseconfig.h
# ============================================================================
generate_sparsehash_config() {
    local config_dir="$SCRIPT_DIR/external/sparsehash/src/sparsehash/internal"
    local config_file="$config_dir/sparseconfig.h"
    
    if [ -f "$config_file" ]; then
        log_info "sparsehash sparseconfig.h already exists."
        return
    fi
    
    log_info "Generating sparsehash sparseconfig.h..."
    mkdir -p "$config_dir"
    
    cat > "$config_file" << 'EOF'
/*
 * sparseconfig.h for sparsehash - generated for CMake build on Linux/GCC
 * NOTE: This file is for internal use only.
 *       Do not use these #defines in your own program!
 */

/* Namespace for Google classes */
#define GOOGLE_NAMESPACE  ::google

/* the location of the header defining hash functions */
#define HASH_FUN_H  <functional>

/* the location of <unordered_map> */
#define HASH_MAP_H  <unordered_map>

/* the namespace of the hash<> function */
#define HASH_NAMESPACE  std

/* the location of <unordered_set> */
#define HASH_SET_H  <unordered_set>

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* define if the compiler implements namespaces */
#define HAVE_NAMESPACES 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/utsname.h> header file. */
#define HAVE_SYS_UTSNAME_H 1

/* Define to 1 if the system has the type `uint16_t'. */
#define HAVE_UINT16_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* define if the compiler supports unordered_{map,set} */
#define HAVE_UNORDERED_MAP 1

/* Define to 1 if the system has the type `u_int16_t'. */
#define HAVE_U_INT16_T 1

/* Define to 1 if the system has the type `__uint16'. */
#undef HAVE___UINT16

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* The system-provided hash function including the namespace. */
#define SPARSEHASH_HASH  std::hash

/* The system-provided hash function, in namespace HASH_NAMESPACE. */
#define SPARSEHASH_HASH_NO_NAMESPACE  hash

/* Stops putting the code inside the Google namespace */
#define _END_GOOGLE_NAMESPACE_  }

/* Puts following code inside the Google namespace */
#define _START_GOOGLE_NAMESPACE_  namespace google {
EOF
    
    log_info "sparsehash sparseconfig.h generated."
}

# ============================================================================
# Step 4: Update CMakeLists.txt for OPIC config path
# ============================================================================
update_cmake() {
    local cmake_file="$SCRIPT_DIR/CMakeLists.txt"
    
    # Check if already patched
    if grep -q 'stubs/opic.*config.h' "$cmake_file" 2>/dev/null; then
        log_info "CMakeLists.txt already patched."
        return
    fi
    
    log_info "Patching CMakeLists.txt for OPIC config path..."
    
    # Replace the opic include directories
    sed -i 's|target_include_directories(opic PUBLIC|target_include_directories(opic PUBLIC\n    ${CMAKE_SOURCE_DIR}/stubs/opic  # config.h (must be first)|' "$cmake_file"
    
    log_info "CMakeLists.txt patched."
}

# ============================================================================
# Step 5: CMake Configure and Build
# ============================================================================
cmake_build() {
    local build_dir="$SCRIPT_DIR/build"
    local nproc=$(nproc 2>/dev/null || echo 4)
    
    log_info "Configuring CMake..."
    cmake -S "$SCRIPT_DIR" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
    
    log_info "Building with $nproc parallel jobs..."
    cmake --build "$build_dir" -j"$nproc"
    
    log_info "Build completed successfully!"
}

# ============================================================================
# Step 6: Run tests
# ============================================================================
run_tests() {
    log_info "Running tests..."
    "$SCRIPT_DIR/build/hashmap_test"
}

# ============================================================================
# Main
# ============================================================================
main() {
    log_info "Starting hashmap_bench build process..."
    
    init_submodules
    generate_opic_config
    generate_sparsehash_config
    update_cmake
    cmake_build
    run_tests
    
    echo ""
    log_info "============================================"
    log_info "Build successful!"
    log_info "  Benchmark: ./build/hashmap_bench"
    log_info "  Test:      ./build/hashmap_test"
    log_info "============================================"
}

main "$@"
