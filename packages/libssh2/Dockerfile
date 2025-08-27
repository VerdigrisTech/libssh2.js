# hadolint global ignore=DL3008,DL3059
FROM node:lts AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV EMSDK_VERSION=latest
ENV OPENSSL_VERSION=1.1.1w
ENV WORKSPACE=/workspace

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    curl \
    tar \
    xz-utils \
    jq \
    && rm -rf /var/lib/apt/lists/*

# Set up Emscripten environment variables
ENV EMSDK="${WORKSPACE}/emsdk"
ENV PATH="${WORKSPACE}/emsdk/upstream/emscripten:${WORKSPACE}/emsdk/upstream/bin:${PATH}"

# Install Emscripten
WORKDIR ${WORKSPACE}
RUN git clone https://github.com/emscripten-core/emsdk.git
WORKDIR ${WORKSPACE}/emsdk
RUN ./emsdk install $EMSDK_VERSION && \
    ./emsdk activate $EMSDK_VERSION

WORKDIR ${WORKSPACE}

# Install pnpm
RUN npm install -g pnpm@latest

COPY . .
RUN pnpm install --frozen-lockfile
RUN pnpm build

# # Download and build OpenSSL for WebAssembly
# RUN cd ${WORKSPACE} && \
#     curl -fsSL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz | tar -xz && \
#     cd openssl-${OPENSSL_VERSION} && \
#     . ${WORKSPACE}/emsdk/emsdk_env.sh && \
#     CC=emcc AR=emar RANLIB=emranlib ./Configure linux-generic32 \
#         -no-asm \
#         -no-threads \
#         -no-engine \
#         -no-hw \
#         -no-weak-ssl-ciphers \
#         -no-dtls \
#         -no-shared \
#         --prefix=${WORKSPACE}/openssl-build && \
#     make -j$(nproc) && \
#     make install

# # Download and build libssh2 for WebAssembly
# RUN git clone https://github.com/libssh2/libssh2.git && \
#     cd libssh2 && \
#     mkdir build-wasm && \
#     cd build-wasm && \
#     emcmake cmake .. \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DBUILD_SHARED_LIBS=OFF \
#         -DBUILD_EXAMPLES=OFF \
#         -DBUILD_TESTING=OFF \
#         -DENABLE_ZLIB_COMPRESSION=OFF \
#         -DCRYPTO_BACKEND=OpenSSL \
#         -DOPENSSL_ROOT_DIR=${WORKSPACE}/openssl-build \
#         -DOPENSSL_INCLUDE_DIR=${WORKSPACE}/openssl-build/include \
#         -DOPENSSL_CRYPTO_LIBRARY=${WORKSPACE}/openssl-build/lib/libcrypto.a \
#         -DOPENSSL_SSL_LIBRARY=${WORKSPACE}/openssl-build/lib/libssl.a \
#         -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/libssh2/install && \
#     emmake make -j$(nproc) && \
#     emmake make install

# Copy the SSH2 bindings
# COPY src/libssh2-bindings.c ${WORKSPACE}/src/libssh2-bindings.c

# Compile the WebAssembly module with bindings
# RUN emcc src/libssh2-bindings.c \
#     -o libssh2.js \
#     -I$WORKSPACE/libssh2/install/include \
#     -L$WORKSPACE/libssh2/install/lib \
#     -L$WORKSPACE/openssl-build/lib \
#     -lssh2 -lssl -lcrypto \
#     -s MODULARIZE=1 \
#     -s EXPORT_ES6=1 \
#     -s ENVIRONMENT=web \
#     -s EXPORTED_FUNCTIONS='["_malloc","_free"]' \
#     -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap","getValue","setValue","FS","HEAPU8"]' \
#     -s ALLOW_MEMORY_GROWTH=1 \
#     -s INITIAL_MEMORY=32MB \
#     -s STACK_SIZE=2MB \
#     -s NO_EXIT_RUNTIME=1 \
#     -s ASSERTIONS=1 \
#     -s SAFE_HEAP=0 \
#     -s DISABLE_EXCEPTION_CATCHING=0 \
#     -O3 \
#     --bind

# Keep the build output minimal
FROM scratch

COPY --from=builder /workspace/libssh2.js /libssh2.js
COPY --from=builder /workspace/libssh2.wasm /libssh2.wasm
