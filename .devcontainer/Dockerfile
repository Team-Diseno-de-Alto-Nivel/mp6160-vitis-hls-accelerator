FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    # Cross-toolchain for the ARM64 driver in src/program,
    # without which `make program` (and therefore `make all`) cannot run here.
    gcc-aarch64-linux-gnu \
    # Pillow/numpy back the RAW <-> JPG helpers in scripts/.
    python3 \
    python3-pil \
    python3-numpy \
    && rm -rf /var/lib/apt/lists/*

# Pre-compile SystemC 2.3.4 so project builds skip the download step
ENV SYSTEMC_HOME=/opt/systemc
RUN curl -L https://github.com/accellera-official/systemc/archive/refs/tags/2.3.4.tar.gz \
        | tar xz -C /tmp \
    && cmake -S /tmp/systemc-2.3.4 -B /tmp/systemc-build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_INSTALL_PREFIX=${SYSTEMC_HOME} \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_SOURCE_DOCUMENTATION=OFF \
    && cmake --build /tmp/systemc-build -j$(nproc) \
    && cmake --install /tmp/systemc-build \
    && rm -rf /tmp/systemc-2.3.4 /tmp/systemc-build

# ── gem5 (virtual-prototype track) ────────────────────────────────────────────
# The accelerator is compiled *into* gem5 via `scons EXTRAS=<repo>/src`, so gem5
# needs its own source tree (a gem5.opt on PATH is not enough). The image ships
# the source tree + gem5's build dependencies with GEM5_ROOT pre-set, so inside
# the container the flow is just:
#     make gem5     # first-time scons build of build/ARM/gem5.opt (~30-60 min)
#     make run-vp   # cross-compile the driver, then run the system
# The scons build is NOT run at image-build time: it links this repo's src/,
# which is only mounted at container runtime. Pin a gem5 tag known to expose the
# SystemC/TLM bridge API the config uses (Gem5ToTlmBridge32, SystemC_ScModule).
ENV GEM5_ROOT=/opt/gem5
RUN apt-get update && apt-get install -y \
    scons \
    m4 \
    python3-dev \
    pkg-config \
    zlib1g zlib1g-dev \
    libprotobuf-dev protobuf-compiler libprotoc-dev \
    libboost-all-dev \
    libpng-dev \
    libgoogle-perftools-dev \
    && rm -rf /var/lib/apt/lists/* \
    && git clone --depth 1 --branch v24.0.0.0 https://github.com/gem5/gem5.git ${GEM5_ROOT}
