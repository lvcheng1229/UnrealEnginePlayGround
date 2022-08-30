#!/bin/bash

# Build instructions:
#
# 1. Make sure LINUX_MULTIARCH_ROOT is set. Ie:
#   export LINUX_MULTIARCH_ROOT=${UE_SDKS_ROOT}/HostLinux/Linux_x64/v17_clang-10.0.1-centos7/
#  or
#   export LINUX_MULTIARCH_ROOT=/epic/v16_clang-9.0.1-centos7
#
# 2. Run this script
#   ./UE4_Build_ShaderConductor_Linux.sh
#
# 3. Binaries should be in Build-RelWithDebInfo.x86_64-unknown-linux-gnu/Lib/ directory

set -eu

SCRIPT_DIR=$(cd "$(dirname "$BASH_SOURCE")" ; pwd)
THIRD_PARTY=$(cd "${SCRIPT_DIR}/.." ; pwd)

# Get num of cores
export CORES=$(getconf _NPROCESSORS_ONLN)
echo "Using ${CORES} cores for building"

BuildShaderConductor()
{
    export ARCH=$1
    export FLAVOR=$2
    local BUILD_DIR=${SCRIPT_DIR}/Build-${FLAVOR}.${ARCH}

    echo "Building ${ARCH}"
    rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}

    pushd ${BUILD_DIR}

    # In CrossCompile.cmake, it's checking for the existence of
    #   ${LLVM_${target_name}_BUILD}  ; which is 'NATIVE' in this case
    # And if it doesn't exist, it launches a couple cmake instances
    # to configure the targets and cross toolchain flags, etc which
    # overwrites our cross toolchain information. We create the
    # NATIVE dir here and it'll skip all that stuff.
    mkdir NATIVE

    set -x
    cmake -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="/tmp/__cmake_toolchain.cmake" \
      -DCMAKE_MAKE_PROGRAM=$(which ninja) \
      -DCMAKE_BUILD_TYPE=${FLAVOR} \
      -DSC_ARCH_NAME=x64 \
      -DPYTHON_EXECUTABLE=$(which python3) \
      -DPython3_EXECUTABLE=$(which python3) \
      -DSPIRV_CROSS_ENABLE_TESTS=OFF \
      -DSC_EXPLICIT_DLLSHUTDOWN=ON \
      -DDXC_EXPLICIT_DLLSHUTDOWN=ON \
      ${SCRIPT_DIR}/ShaderConductor
    set +x

    echo
    #VERBOSE=1 make -j ${CORES} --no-print-directory
    ninja llvm-tblgen clang-tblgen
    mkdir -p ./NATIVE/bin
    cp ./External/DirectXShaderCompiler/bin/* ./NATIVE/bin
    ninja
    echo
    
    # Copy output into Engine/Binaries folder
    local DST_DIR="../../../../Binaries/ThirdParty/ShaderConductor/Linux/${ARCH}"
    cp -vf "./Lib/libdxcompiler.so" "${DST_DIR}/libdxcompiler.so"
    cp -vf "./Lib/libShaderConductor.so" "${DST_DIR}/libShaderConductor.so"
    cp -vf "./Bin/ShaderConductorCmd" "${DST_DIR}/ShaderConductorCmd"

    popd
}

( cat <<_EOF_
  ## autogenerated by ${BASH_SOURCE} script
  SET(LINUX_MULTIARCH_ROOT \$ENV{LINUX_MULTIARCH_ROOT})
  SET(ARCHITECTURE_TRIPLE \$ENV{ARCH})

  message (STATUS "LINUX_MULTIARCH_ROOT is '\${LINUX_MULTIARCH_ROOT}'")
  message (STATUS "ARCHITECTURE_TRIPLE is '\${ARCHITECTURE_TRIPLE}'")

  SET(CMAKE_CROSSCOMPILING TRUE)
  SET(CMAKE_SYSTEM_NAME Linux)
  SET(CMAKE_SYSTEM_VERSION 1)

  # sysroot
  SET(CMAKE_SYSROOT \${LINUX_MULTIARCH_ROOT}/\${ARCHITECTURE_TRIPLE})

  SET(CMAKE_LIBRARY_ARCHITECTURE \${ARCHITECTURE_TRIPLE})

  # specify the cross compiler
  SET(CMAKE_C_COMPILER            \${CMAKE_SYSROOT}/bin/clang)
  SET(CMAKE_C_COMPILER_TARGET     \${ARCHITECTURE_TRIPLE})
  SET(CMAKE_C_FLAGS "-fms-extensions -target      \${ARCHITECTURE_TRIPLE}")

  include_directories("${THIRD_PARTY}/Unix/LibCxx/include")
  include_directories("${THIRD_PARTY}/Unix/LibCxx/include/c++/v1")

  set(CMAKE_LINKER_FLAGS "-stdlib=libc++ -L${THIRD_PARTY}/Unix/LibCxx/lib/Unix/\${ARCHITECTURE_TRIPLE}/ ${THIRD_PARTY}/Unix/LibCxx/lib/Unix/\${ARCHITECTURE_TRIPLE}/libc++.a ${THIRD_PARTY}/Unix/LibCxx/lib/Unix/\${ARCHITECTURE_TRIPLE}/libc++abi.a -lpthread")
  set(CMAKE_EXE_LINKER_FLAGS      "\${CMAKE_LINKER_FLAGS}")
  set(CMAKE_MODULE_LINKER_FLAGS   "\${CMAKE_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS   "\${CMAKE_LINKER_FLAGS}")
  #set(CMAKE_STATIC_LINKER_FLAGS   "\${CMAKE_LINKER_FLAGS}")


  SET(CMAKE_CXX_COMPILER          \${CMAKE_SYSROOT}/bin/clang++)
  SET(CMAKE_CXX_COMPILER_TARGET   \${ARCHITECTURE_TRIPLE})
  SET(CMAKE_CXX_FLAGS             "-std=c++1z -fms-extensions")
  # https://stackoverflow.com/questions/25525047/cmake-generator-expression-differentiate-c-c-code
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)

  SET(CMAKE_ASM_COMPILER          \${CMAKE_SYSROOT}/bin/clang)

  SET(CMAKE_FIND_ROOT_PATH        \${LINUX_MULTIARCH_ROOT})

  # hoping to force it to use ar
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

_EOF_
) > /tmp/__cmake_toolchain.cmake

if [ "$#" -eq 1 ] && [ "$1" == "-debug" ]; then
	BuildShaderConductor x86_64-unknown-linux-gnu Debug
else
	BuildShaderConductor x86_64-unknown-linux-gnu RelWithDebInfo
fi