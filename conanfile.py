name: CMake

on:
  push:
    branches: [ master, dev ]
  pull_request:
    branches: [ master ]

env:
  # Customize the CMake build type here (Release, Debug, RelWithDebInfo, etc.)
  BUILD_TYPE: Release

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - uses: actions/checkout@v2

    - name: Setup GCC-11
      run: sudo apt install -y --no-install-recommends gcc-11 g++-11
      
#     - name: Setup Clang-14
#       run: sudo apt-get install -y --no-install-recommends \
#         clang-14 lldb-14 lld-14 clangd-14 \
#         llvm-14-dev libclang-14-dev clang-tidy-14 \
#         libc++-dev

    - name: Setup precompiled libraries
      run: sudo apt install
        libfmt-dev
        libspdlog-dev
        libboost-all-dev
        protobuf-compiler
        libprotoc-dev

    ######### BOOST #########
    - name: Cache Boost
      id: cache-boost
      uses: actions/cache@v2
      with:
        path: ${{github.workspace}}/boost_1_78_0
        key:  boost_1_78_0

    - name: Download and bootstrap Boost
      if: steps. cache-boost.outputs.cache-hit != 'true'
      run: wget https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz
          && tar -zxf boost_1_78_0.tar.gz
          && cd boost_1_78_0 && ./bootstrap.sh --prefix=/usr

    - name: Install Boost
      run: cd boost_1_78_0 && sudo ./b2 install
          && cd ${{github.workspace}}

    ######### ZMQ #########
    - name: Install ZMQ
      run: git clone https://github.com/zeromq/libzmq.git && cd libzmq
            && ./autogen.sh
            && ./configure
            && sudo make -j4 install
            && cd ${{github.workspace}}

    ######### CPPZMQ #########
    - name: Install CPPZMQ
      run: git clone https://github.com/zeromq/cppzmq.git && cd cppzmq
            && mkdir build && cd build
            && cmake ..
            && sudo make -j4 install
            && cd ${{github.workspace}}

    - name: Install Catch2
      run: git clone https://github.com/catchorg/Catch2.git
          && cd Catch2
          && cmake -Bbuild -H. -DBUILD_TESTING=OFF
          && sudo cmake --build build/ --target install
          && cd ${{github.workspace}}

    - name: Configure CMake
      run: cmake -B ${{github.workspace}}/build -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} -DENABLE_TESTING=ON
      env:
        CC: gcc-11
        CXX: g++-11
        BOOST_ROOT: ${{ steps.install-boost.outputs.BOOST_ROOT }}

    - name: Build
      run: cmake --build ${{github.workspace}}/build --config ${{env.BUILD_TYPE}}

    - name: Test
      working-directory: ${{github.workspace}}/build
      run: ctest -C ${{env.BUILD_TYPE}}
