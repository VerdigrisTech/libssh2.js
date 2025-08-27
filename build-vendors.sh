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

# Build specific vendor
build_vendor() {
    local vendor_name="$1"

    case "$vendor_name" in
        "zlib")
            build_zlib
            ;;
        "openssl")
            build_openssl
            ;;
        "libssh2")
            build_libssh2
            ;;
        *)
            log_error "Unknown vendor: $vendor_name"
            log_info "Available vendors: zlib, openssl, libssh2"
            return 1
            ;;
    esac
}

# Build zlib function
build_zlib() {
    if [ -d "$SCRIPT_DIR/vendor/zlib" ]; then
        if [ -f "$EMPORTS/lib/libz.a" ]; then
            log_info "zlib already built, skipping..."
        else
            log_info "Building zlib..."
            cd "$SCRIPT_DIR/vendor/zlib"

            # Clean any previous build
            emmake make distclean 2>/dev/null || true

            # Configure with proper AR for Emscripten
            if ! emconfigure ./configure --static --prefix="$EMPORTS"; then
                log_error "zlib configure failed"
                return 1
            fi

            # Fix AR and ARFLAGS for Emscripten
            sed -i.bak 's/^AR=libtool$/AR=emar/' Makefile
            sed -i.bak2 's/^ARFLAGS=-o$/ARFLAGS=rcs/' Makefile

            if ! emmake make -j4; then
                log_error "zlib build failed"
                return 1
            fi
            
            if ! emmake make install; then
                log_error "zlib install failed"
                return 1
            fi
            
            log_success "zlib build completed"
        fi
    else
        log_error "vendor/zlib directory not found. Run ./download-vendors.sh first."
        return 1
    fi
}

# Build OpenSSL function
build_openssl() {
    if [ -d "$SCRIPT_DIR/vendor/openssl" ]; then
        if [ -f "$EMPORTS/lib/libssl.a" ] && [ -f "$EMPORTS/lib/libcrypto.a" ]; then
            log_info "OpenSSL already built, skipping..."
        else
            log_info "Building OpenSSL..."
            cd "$SCRIPT_DIR/vendor/openssl"
            
            if ! emconfigure ./Configure linux-generic32 -no-asm -no-threads -no-engine -no-hw -no-weak-ssl-ciphers -no-dtls -no-shared --with-zlib-include="$EMPORTS/include" --with-zlib-lib="$EMPORTS/lib" --prefix="$EMPORTS"; then
                log_error "OpenSSL configure failed"
                return 1
            fi

            # Fix CC, AR, and RANLIB in the generated Makefile (they get concatenated incorrectly)
            sed -i.bak "s|CC=\$(CROSS_COMPILE)$(which emcc)|CC=$(which emcc)|" Makefile
            sed -i.bak2 "s|AR=\$(CROSS_COMPILE)$(which emar)|AR=$(which emar)|" Makefile
            sed -i.bak3 "s|RANLIB=\$(CROSS_COMPILE)$(which emranlib)|RANLIB=$(which emranlib)|" Makefile
            
            if ! emmake make build_sw -j4; then
                log_error "OpenSSL build failed"
                return 1
            fi
            
            if ! emmake make install_sw; then
                log_error "OpenSSL install failed"
                return 1
            fi
            
            log_success "OpenSSL build completed"
        fi
    else
        log_warning "vendor/openssl directory not found. Skipping OpenSSL build."
        return 1
    fi
}

# Build libssh2 function
build_libssh2() {
    if [ -d "$SCRIPT_DIR/vendor/libssh2" ]; then
        if [ -f "$EMPORTS/lib/libssh2.a" ]; then
            log_info "libssh2 already built, skipping..."
        else
            log_info "Building libssh2..."
            cd "$SCRIPT_DIR/vendor/libssh2"
            mkdir -p build-wasm
            cd build-wasm
            
            if ! emcmake cmake .. \
                -DCMAKE_BUILD_TYPE=Release \
                -DBUILD_SHARED_LIBS=OFF \
                -DBUILD_EXAMPLES=OFF \
                -DBUILD_TESTING=OFF \
                -DENABLE_ZLIB_COMPRESSION=ON \
                -DCRYPTO_BACKEND=OpenSSL \
                -DOPENSSL_ROOT_DIR="$EMPORTS" \
                -DOPENSSL_INCLUDE_DIR="$EMPORTS/include" \
                -DOPENSSL_CRYPTO_LIBRARY="$EMPORTS/lib/libcrypto.a" \
                -DOPENSSL_SSL_LIBRARY="$EMPORTS/lib/libssl.a" \
                -DZLIB_INCLUDE_DIR="$EMPORTS/include" \
                -DZLIB_LIBRARY="$EMPORTS/lib/libz.a" \
                -DCMAKE_INSTALL_PREFIX="$EMPORTS"; then
                log_error "libssh2 cmake configure failed"
                return 1
            fi
            
            if ! emmake make -j$(nproc); then
                log_error "libssh2 build failed"
                return 1
            fi
            
            if ! emmake make install; then
                log_error "libssh2 install failed"
                return 1
            fi
            
            log_success "libssh2 build completed"
        fi
    else
        log_warning "vendor/libssh2 directory not found. Skipping libssh2 build."
        return 1
    fi
}

# Main function
main() {
    local vendors_to_build=("$@")

    log_info "Starting vendor build process"

    # Create emscripten directory if it doesn't exist
    mkdir -p "$EMPORTS"

    # Check Emscripten availability
    check_emscripten

    # If no specific vendors specified, build all
    if [ ${#vendors_to_build[@]} -eq 0 ]; then
        vendors_to_build=("zlib" "openssl" "libssh2")
    fi

    local failed_builds=()
    local successful_builds=()

    # Build specified vendors
    for vendor in "${vendors_to_build[@]}"; do
        if build_vendor "$vendor"; then
            successful_builds+=("$vendor")
        else
            failed_builds+=("$vendor")
        fi
    done

    # Summary
    echo "========================================="
    log_info "Build Summary:"

    if [ ${#successful_builds[@]} -gt 0 ]; then
        log_success "Successfully built: ${successful_builds[*]}"
    fi

    if [ ${#failed_builds[@]} -gt 0 ]; then
        log_error "Failed to build: ${failed_builds[*]}"
        return 1
    else
        log_success "All requested vendor builds completed successfully!"
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [vendor1] [vendor2] [...] [--help]"
        echo ""
        echo "Builds vendor libraries for Emscripten/WebAssembly compilation"
        echo ""
        echo "Available vendors:"
        echo "  zlib     - Compression library"
        echo "  openssl  - SSL/TLS library (requires zlib)"
        echo "  libssh2  - SSH2 library (requires zlib and openssl)"
        echo ""
        echo "Examples:"
        echo "  $0                    # Build all vendors"
        echo "  $0 zlib               # Build only zlib"
        echo "  $0 zlib openssl       # Build zlib and openssl"
        echo "  $0 openssl libssh2    # Build openssl and libssh2"
        exit 0
        ;;
    "")
        main
        ;;
    *)
        main "$@"
        ;;
esac
