cmake_minimum_required (VERSION 3.13...3.22)

project(OrderBook)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

find_package(folly REQUIRED)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC flags
    set(PERF_FLAGS 
        -O3
        -march=native
        -flto
        -funroll-loops
        -finline-functions
)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang flags
    set(PERF_FLAGS
        -O3
        -march=native
        -flto
        -funroll-loops
        -finline-functions
)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC flags
    set(PERF_FLAGS
        /O3          # Maximum optimization (favor speed)
        /Oi          # Enable intrinsic functions
        /Ot          # Favor fast code
        /Oy          # Omit frame pointers
        /GL          # Whole program optimization
        /arch:AVX512   # Use AVX2 instructions (adjust based on target)
)
endif()

include_directories(
    include
    ${PROJECT_SOURCE_DIR}/Connections/include
    ${PROJECT_SOURCE_DIR}/Logger/include
    ${PROJECT_SOURCE_DIR}/Msg/include
    ${PROJECT_SOURCE_DIR}/OrderBook/include
    ${PROJECT_SOURCE_DIR}/Utils/include
    ${PROJECT_SOURCE_DIR}/MarketDataSimulator/include
)

# Platform-specific include directories
if (WIN32)
    include_directories(
        include
    )
elseif(UNIX AND NOT APPLE)
    include_directories(
        /usr/local/include
    )
elseif(APPLE)
    include_directories(
        /usr/local/include
        /opt/homebrew/include
    )
endif()

# Library search path
link_directories(${PROJECT_SOURCE_DIR}/lib)

# Add the source files
file(GLOB CONNECTIONS_SRC "Connections/*.cpp")
file(GLOB LOGGER_SRC "Logger/*.cpp")
file(GLOB MSG_SRC "Msg/*.cpp")
file(GLOB BOOK_SRC "OrderBook/*.cpp")
file(GLOB UTILS_SRC "Utils/*.cpp")
file(GLOB SIMULATOR_SRC "MarketDataSimulator/*.cpp")

set (ORDERBOOK_SRC
    ${CONNECTIONS_SRC}
    ${LOGGER_SRC}
    ${MSG_SRC}
    ${UTILS_SRC}
    ${BOOK_SRC}
)

set (MKT_SIMULATOR_SRC
    ${CONNECTIONS_SRC}
    ${LOGGER_SRC}
    ${MSG_SRC}
    ${UTILS_SRC}
    ${SIMULATOR_SRC}
)

# Add the executable
add_executable(OrderBook ${ORDERBOOK_SRC})

add_executable(MarketDataSimulator ${MKT_SIMULATOR_SRC})



