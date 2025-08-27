#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EMPORTS="$SCRIPT_DIR/tmp/emscripten"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if Emscripten is available and try to source it
check_emscripten() {
    if ! command -v emcc &> /dev/null; then
        # Try to find and source emsdk_env.sh
        local emsdk_paths=(
            "$HOME/emsdk/emsdk_env.sh"
            "$SCRIPT_DIR/emsdk/emsdk_env.sh"
            "$SCRIPT_DIR/../emsdk/emsdk_env.sh"
            "./emsdk/emsdk_env.sh"
        )

        local found_emsdk=false
        for emsdk_path in "${emsdk_paths[@]}"; do
            if [ -f "$emsdk_path" ]; then
                log_info "Found emsdk_env.sh at $emsdk_path, sourcing..."
                source "$emsdk_path"
                found_emsdk=true
                break
            fi
        done

        # Check again after sourcing
        if ! command -v emcc &> /dev/null; then
            if [ "$found_emsdk" = true ]; then
                log_error "Found emsdk_env.sh but emcc is still not available"
                log_info "Try running: source $emsdk_path"
            else
                log_error "Emscripten is not installed or not in PATH"
                log_info "To install Emscripten:"
                log_info "1. git clone https://github.com/emscripten-core/emsdk.git"
                log_info "2. cd emsdk"
                log_info "3. ./emsdk install latest"
                log_info "4. ./emsdk activate latest"
                log_info "5. source ./emsdk_env.sh"
            fi
            exit 1
        fi
    fi

    log_info "Found Emscripten: $(emcc --version | head -1)"
}

clean() {
    log_info "Cleaning previous builds..."
    rm -rf build dist

    # Create build directory
    mkdir -p build
    mkdir -p dist
}

build_libssh2_wasm() {
    log_info "Building libssh2.js WebAssembly wrapper..."

    if ! emcc src/libssh2-bindings.c \
      -o dist/libssh2.js \
      -I$EMPORTS/include \
      -L$EMPORTS/lib \
      -lssh2 -lssl -lcrypto -lz \
      -s MODULARIZE=1 \
      -s EXPORT_ES6=1 \
      -s ENVIRONMENT=web \
      -s EXPORTED_FUNCTIONS='["_malloc","_free"]' \
      -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","setValue","FS","HEAPU8"]' \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s INITIAL_MEMORY=32MB \
      -s STACK_SIZE=2MB \
      -s NO_EXIT_RUNTIME=1 \
      -s ASSERTIONS=1 \
      -s SAFE_HEAP=0 \
      -s DISABLE_EXCEPTION_CATCHING=0 \
      -O3 \
      --bind; then
        log_error "Failed to build libssh2.js WebAssembly wrapper"
        exit 1
    fi

    log_success "libssh2.js WebAssembly wrapper built successfully"
}

generate_types() {
    log_info "Generating TypeScript declarations..."

    if command -v tsc &> /dev/null; then
        if ! tsc --declaration --emitDeclarationOnly --outDir dist; then
            log_error "Failed to generate TypeScript declarations"
            exit 1
        fi
        log_success "TypeScript declarations generated successfully"
    else
        log_warning "TypeScript compiler not found, skipping type generation"
        log_info "Run 'npm run build:types' manually to generate types"
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [--with-types] [--help]"
        echo ""
        echo "Builds libssh2.js WebAssembly library"
        echo ""
        echo "Options:"
        echo "  --with-types    Generate TypeScript declarations after build"
        echo "  --help          Show this help message"
        exit 0
        ;;
    --with-types)
        check_emscripten
        build_libssh2_wasm
        generate_types
        ;;
    "")
        check_emscripten
        build_libssh2_wasm
        generate_types
        ;;
    *)
        log_error "Unknown argument: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
