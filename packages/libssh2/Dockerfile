# hadolint global ignore=DL3008,DL3059
FROM node:22-trixie-slim AS builder

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

# Keep the build output minimal
FROM scratch

COPY --from=builder /workspace/libssh2.js /libssh2.js
COPY --from=builder /workspace/libssh2.wasm /libssh2.wasm
COPY --from=builder /workspace/libssh2.d.ts /libssh2.d.ts
