set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Target the KV260's actual CPU (quad Cortex-A53, ARMv8.0-A). Correct on the real
# board, and it keeps the compiler from emitting post-v8.0 instructions gem5's SE
# CPU mishandles — notably the BTI/PAC that Ubuntu's -mbranch-protection=standard
# default adds, which the gem5 model executes rather than treating as A53 NOPs.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-a53 -mbranch-protection=none")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -mbranch-protection=none")

# Produce a self-contained ARM64 ELF (no dynamic loader needed).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
