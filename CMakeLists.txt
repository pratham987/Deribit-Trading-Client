cmake_minimum_required(VERSION 3.10)
project(CryptoTrader VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compile options
add_compile_options(-Wall -Wextra -Wpedantic)

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)

# Set include directories
include_directories(
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

# Check if nlohmann/json is available (you may need to adjust this based on your setup)
# If you're using a package manager or have it installed system-wide
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # Include from thirdparty or fetch it
    message(STATUS "nlohmann/json not found in system, using local copy or download")
    
    # You can uncomment and use this to download if needed
    # include(FetchContent)
    # FetchContent_Declare(
    #     json
    #     GIT_REPOSITORY https://github.com/nlohmann/json.git
    #     GIT_TAG v3.11.2
    # )
    # FetchContent_MakeAvailable(json)
endif()

# Check for websocketpp (you'll likely need to adjust this based on your setup)
# WebSocketPP is typically header-only, so you might just specify the include path
find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/client.hpp
    HINTS "${CMAKE_SOURCE_DIR}/thirdparty/websocketpp")

if(WEBSOCKETPP_INCLUDE_DIR)
    include_directories(${WEBSOCKETPP_INCLUDE_DIR})
else()
    message(FATAL_ERROR "websocketpp headers not found")
endif()

# Set source files
set(SOURCES
    main.cpp
    trader.cpp
    websocket.cpp
    logger.cpp
)

# Add executable
add_executable(crypto_trader ${SOURCES})

# Link libraries
target_link_libraries(crypto_trader
    PRIVATE
    ${CURL_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# If using the package-found version of nlohmann/json
if(nlohmann_json_FOUND)
    target_link_libraries(crypto_trader PRIVATE nlohmann_json::nlohmann_json)
endif()

# Install the executable
install(TARGETS crypto_trader DESTINATION bin)

# Testing configuration (if needed)
# enable_testing()
# add_subdirectory(tests)