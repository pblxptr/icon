FROM ubuntu:focal

ARG DEBIAN_FRONTEND=noninteractive

# Install packages available from standard repos
RUN apt-get update -qq && \
    apt-get install -y --no-install-recommends \
        software-properties-common wget git gpg-agent file \
        python3 python3-pip doxygen graphviz ccache cppcheck build-essential \
        neovim emacs nano

# User-settable versions:
# This Dockerfile should support gcc-[7, 8, 9, 10] and clang-[10, 11]
# Earlier versions of clang will require significant modifications to the IWYU section
ARG GCC_VER="11"
ARG LLVM_VER="13"

# Add gcc-${GCC_VER}
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt-get update -qq && \
    apt-get install -y --no-install-recommends gcc-${GCC_VER} g++-${GCC_VER}

# # Add clang-${LLVM_VER}
# RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - 2>/dev/null && \
#     add-apt-repository -y "deb http://apt.llvm.org/focal/ llvm-toolchain-focal main" && \
#     #Add test repository for libstdc++ with C++20 support
#     add-apt-repository -y "ppa:ubuntu-toolchain-r/test" && \
#     # add-apt-repository -y "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-${LLVM_VER} main" && \
#     apt-get update -qq && \
#     apt-get install -y --no-install-recommends \
#         clang-${LLVM_VER} lldb-${LLVM_VER} lld-${LLVM_VER} clangd-${LLVM_VER} \
#         llvm-${LLVM_VER}-dev libclang-${LLVM_VER}-dev clang-tidy-${LLVM_VER} \
#         libc++-dev

# Add current cmake/ccmake, from Kitware
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null \
        | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main' && \
    apt-get update -qq && \
    apt-get install -y --no-install-recommends cmake cmake-curses-gui

# Set the default clang-tidy, so CMake can find it
# RUN update-alternatives --install /usr/bin/clang-tidy clang-tidy $(which clang-tidy-${LLVM_VER}) 1

# # Install include-what-you-use
# ENV IWYU /home/iwyu
# ENV IWYU_BUILD ${IWYU}/build
# ENV IWYU_SRC ${IWYU}/include-what-you-use
# RUN mkdir -p ${IWYU_BUILD} && \
#     git clone --branch clang_${LLVM_VER} \
#         https://github.com/include-what-you-use/include-what-you-use.git \
#         ${IWYU_SRC}
# RUN CC=clang-${LLVM_VER} CXX=clang++-${LLVM_VER} cmake -S ${IWYU_SRC} \
#         -B ${IWYU_BUILD} \
#         -G "Unix Makefiles" -DCMAKE_PREFIX_PATH=/usr/lib/llvm-${LLVM_VER} && \
#     cmake --build ${IWYU_BUILD} -j && \
#     cmake --install ${IWYU_BUILD}

# # Per https://github.com/include-what-you-use/include-what-you-use#how-to-install:
# # `You need to copy the Clang include directory to the expected location before
# #  running (similarly, use include-what-you-use -print-resource-dir to learn
# #  exactly where IWYU wants the headers).`
# RUN mkdir -p $(include-what-you-use -print-resource-dir 2>/dev/null)
# RUN ln -s $(readlink -f /usr/lib/clang/${LLVM_VER}/include) \
#     $(include-what-you-use -print-resource-dir 2>/dev/null)/include

## Cleanup cached apt data we don't need anymore
#RUN apt-get clean && \
#    rm -rf /var/lib/apt/lists/*

# Set gcc-${GCC_VER} as default gcc
RUN update-alternatives --install /usr/bin/gcc gcc $(which gcc-${GCC_VER}) 100
RUN update-alternatives --install /usr/bin/g++ g++ $(which g++-${GCC_VER}) 100

# Set clang-${LLVM_VER} as default clang
# RUN update-alternatives --install /usr/bin/clang clang $(which clang-${LLVM_VER}) 100
# RUN update-alternatives --install /usr/bin/clang++ clang++ $(which clang++-${LLVM_VER}) 100

# Allow the user to set compiler defaults
ARG USE_CLANG
# # if --build-arg USE_CLANG=1, set CC to 'clang' or set to null otherwise.
# ENV CC=${USE_CLANG:+"clang"}
# ENV CXX=${USE_CLANG:+"clang++"}
# # if CC is null, set it to 'gcc' (or leave as is otherwise).
# ENV CC=${CC:-"gcc"}
# ENV CXX=${CXX:-"g++"}

### Install additional tools ###
RUN apt-get install -y --no-install-recommends \
  autoconf \
  autotools-dev \
  automake \
  pkg-config \
  m4 \
  libtool \
  libtool-bin

### Install precompiled libraries ###
RUN apt-get install -y --no-install-recommends \
  libfmt-dev \
  libspdlog-dev \
  protobuf-compiler \
  libprotoc-dev \
  libboost-all-dev

### name: Install Catch2
RUN git clone https://github.com/catchorg/Catch2.git \
    && cd Catch2 \
    && cmake -B build -H. -DBUILD_TESTING=OFF \
    && cmake --build build/ --target install

### Install ZMQ ###
RUN git clone https://github.com/zeromq/libzmq.git && cd libzmq \
  && ./autogen.sh \
  && ./configure \
  && make -j4 install

### Install CPPZMQ ###
RUN git clone https://github.com/zeromq/cppzmq.git && cd cppzmq \
  && mkdir build && cd build \
  && cmake .. -DCPPZMQ_BUILD_TESTS=OFF \
  && make -j4 install

### Install BOOST 1.77 ###
ENV BOOST /home/boost
# ENV BOOST_TOOLSET=${USE_CLANG:+"clang"}
# ENV BOOST_TOOLSET=${BOOST_TOOLSET:-"gcc"}
# ENV BOOST_STDLIB=${USE_CLANG:+"libc++"}
# ENV BOOST_STDLIB=${BOOST_STDLIB:-"libstdc++"}

RUN mkdir ${BOOST} && cd ${BOOST} && wget https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz \
          && tar -zxf boost_1_77_0.tar.gz \
          && cd boost_1_77_0 && ./bootstrap.sh cxxstd=20 --with-toolset=${BOOST_TOOLSET} --without-libraries=mpi,python,graph_parallel --prefix=/usr \
          && ./b2 cxxstd=20 define=BOOST_SYSTEM_NO_DEPRECATED install

# Include project
ADD . /icon
WORKDIR /icon

CMD ["/bin/bash"]
