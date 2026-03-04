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
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_ok() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Get number of parallel jobs
get_nproc() { nproc 2>/dev/null || echo 4; }

# Count compilation targets in build directory
count_targets() {
    local build_dir="$1"
    # Count all compile commands from compile_commands.json if available
    if [ -f "$build_dir/compile_commands.json" ]; then
        grep -c '"command":' "$build_dir/compile_commands.json" 2>/dev/null || echo 100
    else
        # Fallback: count from build.make files
        find "$build_dir" -name "build.make" -exec grep -h "\.cpp\.o:\|\.c\.o:" {} \; 2>/dev/null | \
            grep -v "flags.make" | grep -v "compiler_depend" | sort -u | wc -l
    fi
}

# ============================================================================
# Compile with single-line progress display
# Shows [current/total] format, single-line update in terminal
# ============================================================================
compile_progress() {
    local prefix="$1"
    local total="$2"
    
    if [ -t 1 ]; then
        stdbuf -oL -eL awk -v prefix="$prefix" -v total="$total" -v BLUE='\033[0;34m' -v NC='\033[0m' '
        BEGIN { count = 0 }
        /Building C[XX]+ object/ {
            count++
            match($0, /object [^ ]+/)
            file = substr($0, RSTART+7, RLENGTH-7)
            printf "\r\033[K%s[INFO]%s %s [%d/%d] %s", BLUE, NC, prefix, count, total, file
            fflush(stdout)
        }
        /Linking/ {
            match($0, /(executable|shared library) ([^ ]+)/)
            file = substr($0, RSTART, RLENGTH)
            printf "\r\033[K%s[INFO]%s %s [%d/%d] linking %s", BLUE, NC, prefix, count, total, file
            fflush(stdout)
        }
        END {
            printf "\r\033[K%s[INFO]%s %s [%d/%d] done.\n", BLUE, NC, prefix, count, (total > count ? total : count)
        }
        '
    else
        awk -v prefix="$prefix" -v total="$total" '
        BEGIN { count = 0 }
        /Building C[XX]+ object/ {
            count++
            match($0, /object [^ ]+/)
            file = substr($0, RSTART+7, RLENGTH-7)
            printf "[INFO] %s [%d/%d] %s\n", prefix, count, total, file
        }
        END {
            printf "[INFO] %s [%d/%d] done.\n", prefix, count, (total > count ? total : count)
        }
        '
    fi
}

# ============================================================================
# Step 1: Initialize all submodules
# ============================================================================
init_submodules() {
    log_info "Initializing submodules..."
    
    git submodule sync --quiet
    
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
    
    local total=${#submodules[@]}
    local current=0
    local fetched=0
    
    for sm in "${submodules[@]}"; do
        current=$((current + 1))
        local sm_name=$(basename "$sm")
        
        if [ ! -f "$sm/.git" ] || [ -z "$(ls -A "$sm" 2>/dev/null | grep -v '^.git$')" ]; then
            fetched=$((fetched + 1))
            printf "\r\033[K${BLUE}[INFO]${NC} Submodules [%d/%d] Fetching: %s" "$current" "$total" "$sm_name"
            git submodule update --init -- "$sm" 2>/dev/null || {
                cd "$sm"
                git fetch origin 2>/dev/null || true
                git checkout HEAD 2>/dev/null || true
                cd "$SCRIPT_DIR"
            }
        else
            printf "\r\033[K${BLUE}[INFO]${NC} Submodules [%d/%d] Checking: %s" "$current" "$total" "$sm_name"
        fi
    done
    
    printf "\r\033[K${BLUE}[INFO]${NC} Submodules [%d/%d] done.\n" "$total" "$total"
    
    if [ $fetched -gt 0 ]; then
        log_ok "Fetched $fetched submodule(s)"
    fi
}

# ============================================================================
# Step 2: Generate OPIC config.h
# ============================================================================
generate_opic_config() {
    local config_dir="$SCRIPT_DIR/stubs/opic"
    local config_file="$config_dir/config.h"
    
    if [ -f "$config_file" ]; then
        return
    fi
    
    log_info "Generating OPIC config.h..."
    mkdir -p "$config_dir"
    
    cat > "$config_file" << 'EOF'
/* config.h for OPIC - generated for CMake build */

#ifndef OPIC_CONFIG_H
#define OPIC_CONFIG_H

#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC 1
#define HAVE_MEMCMP 1
#define HAVE_MMAP 1
#define HAVE_STDBOOL_H 1
#define HAVE_PTHREAD_H 1
#define HAVE_PTHREAD 1
#define PACKAGE "opic"
#define PACKAGE_VERSION "0.8.0"
#define VERSION "0.8.0"

#endif /* OPIC_CONFIG_H */
EOF
    
    log_ok "OPIC config.h generated"
}

# ============================================================================
# Step 3: Generate sparsehash sparseconfig.h
# ============================================================================
generate_sparsehash_config() {
    local config_dir="$SCRIPT_DIR/external/sparsehash/src/sparsehash/internal"
    local config_file="$config_dir/sparseconfig.h"
    
    if [ -f "$config_file" ]; then
        return
    fi
    
    log_info "Generating sparsehash config..."
    mkdir -p "$config_dir"
    
    cat > "$config_file" << 'EOF'
/* sparseconfig.h for sparsehash */

#define GOOGLE_NAMESPACE  ::google
#define HASH_FUN_H  <functional>
#define HASH_MAP_H  <unordered_map>
#define HASH_NAMESPACE  std
#define HASH_SET_H  <unordered_set>
#define HAVE_INTTYPES_H 1
#define HAVE_LONG_LONG 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_NAMESPACES 1
#define HAVE_PTHREAD 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_UINT16_T 1
#define HAVE_UNISTD_H 1
#define HAVE_UNORDERED_MAP 1
#define HAVE_U_INT16_T 1
#define STDC_HEADERS 1
#define SPARSEHASH_HASH  std::hash
#define SPARSEHASH_HASH_NO_NAMESPACE  hash
#define _END_GOOGLE_NAMESPACE_  }
#define _START_GOOGLE_NAMESPACE_  namespace google {
EOF
    
    log_ok "sparsehash config generated"
}

# ============================================================================
# Step 4: Update CMakeLists.txt for OPIC config path
# ============================================================================
update_cmake() {
    local cmake_file="$SCRIPT_DIR/CMakeLists.txt"
    
    if grep -q 'stubs/opic.*config.h' "$cmake_file" 2>/dev/null; then
        return
    fi
    
    log_info "Patching CMakeLists.txt..."
    sed -i 's|target_include_directories(opic PUBLIC|target_include_directories(opic PUBLIC\n    ${CMAKE_SOURCE_DIR}/stubs/opic  # config.h|' "$cmake_file"
    log_ok "CMakeLists.txt patched"
}

# ============================================================================
# Step 5: CMake Configure and Build
# ============================================================================
cmake_build() {
    local build_dir="$SCRIPT_DIR/build"
    local nproc=$(get_nproc)
    
    log_info "Configuring CMake..."
    cmake -S "$SCRIPT_DIR" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -Wno-dev 2>/dev/null
    
    local total=$(count_targets "$build_dir")
    [ "$total" -eq 0 ] && total=100
    
    log_info "Building with $nproc jobs..."
    cmake --build "$build_dir" -j"$nproc" -- 2>&1 | compile_progress "Building" "$total"
    
    log_ok "Build complete"
}

# ============================================================================
# Step 6: Run tests
# ============================================================================
run_tests() {
    log_info "Running tests..."
    "$SCRIPT_DIR/build/hashmap_test"
    log_ok "Tests passed"
}

# ============================================================================
# Main
# ============================================================================
main() {
    log_info "Starting hashmap_bench build..."
    
    init_submodules
    generate_opic_config
    generate_sparsehash_config
    update_cmake
    cmake_build
    run_tests
    
    echo ""
    log_ok "Build successful!"
    echo "  Benchmark: ./build/hashmap_bench"
    echo "  Test:      ./build/hashmap_test"
}

main "$@"
