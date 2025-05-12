# Deribit Trading Client

A C++ application for trading on the Deribit cryptocurrency derivatives exchange using both REST API and WebSocket connections.

## Overview

This project provides a complete trading client for interacting with the Deribit cryptocurrency derivatives exchange API. The application enables traders to execute orders, manage positions, and stream real-time market data through a unified interface. It's designed to showcase high-performance cryptocurrency trading capabilities while providing detailed performance metrics.

The client supports both synchronous REST API calls for account operations and trades, as well as asynchronous WebSocket connections for real-time market data streaming. The implementation emphasizes reliability, with features like automatic reconnection, authentication handling, and comprehensive error reporting.

Key features include:

- **REST API Integration**: Complete interface for account operations, order placement, and market data retrieval
- **WebSocket Support**: Real-time market data streaming with subscription management
- **Performance Monitoring**: Detailed latency tracking for all operations
- **Comprehensive Logging**: Structured logging system with different severity levels
- **Interactive CLI**: User-friendly command-line interface for trading operations
- **Authentication Management**: Automatic token handling and refresh
- **Error Handling**: Robust error recovery and reporting mechanisms

This implementation serves as both a practical trading tool and a reference implementation for developers looking to integrate with cryptocurrency exchanges.

## Components

The application consists of several key components:

- **Trader**: Handles REST API communication with Deribit
- **WebSocket Client**: Manages real-time data streams
- **Logger**: Provides detailed logging with performance metrics
- **Main Application**: CLI interface for user interaction

## Prerequisites

- C++17 compatible compiler
- CMake (3.10 or higher)
- libcurl
- Boost ASIO
- OpenSSL
- WebSocket++ library
- nlohmann/json library

## Building the Project

### 1. Install dependencies

#### Ubuntu/Debian:
```bash
sudo apt update
sudo apt install git cmake build-essential libcurl4-openssl-dev libboost-all-dev libssl-dev
```

#### Fedora/RHEL:
```bash
sudo dnf install git cmake gcc-c++ libcurl-devel boost-devel openssl-devel
```

#### macOS (using Homebrew):
```bash
brew install cmake curl boost openssl
```

### 2. Install WebSocket++ and nlohmann/json

WebSocket++ is a header-only library:

```bash
git clone https://github.com/zaphoyd/websocketpp.git
sudo cp -r websocketpp/websocketpp /usr/local/include/
```

nlohmann/json is also header-only:

```bash
git clone https://github.com/nlohmann/json.git
sudo cp -r json/single_include/nlohmann /usr/local/include/
```

### 3. Build the project

```bash
mkdir build
cd build
cmake ..
make
```

## Configuration

Before running the application, create a `config.json` file in the project root with your Deribit API credentials:

```json
{
  "clientId": "YOUR_CLIENT_ID",
  "clientSecret": "YOUR_CLIENT_SECRET"
}
```

**Note**: Update the path to the config file in `main.cpp` if you place it somewhere other than `/home/pratham/gq_task/config.json`.

## Running the Application

From the build directory:

```bash
./deribit_client
```

## Using the Application

After launching, you'll see a menu with these options:

1. **Buy** - Place a buy order
2. **Sell** - Place a sell order
3. **Cancel** - Cancel an existing order
4. **Modify** - Modify an existing order
5. **View Current Positions** - Check your open positions
6. **Order Book** - View the order book for an instrument
7. **Market data streaming** - Subscribe/unsubscribe to WebSocket channels
8. **Exit** - Exit the application

### WebSocket Channels

For market data streaming (option 7), you can subscribe to channels like:

- Public channels:
  - `book.BTC-PERPETUAL.100ms` - Order book updates
  - `ticker.BTC-PERPETUAL.100ms` - Ticker updates
  - `trades.BTC-PERPETUAL.100ms` - Trade updates

- Private channels:
  - `user.orders.BTC-PERPETUAL.100ms` - User order updates
  - `user.trades.BTC-PERPETUAL.100ms` - User trade updates

## Logs

Logs are stored in the directory specified in `logger.cpp`. By default, they will be saved to `/home/pratham/gq_task/v2/` with filenames following the pattern `log_YYYYMMDD_HHMMSS.txt`.

## Performance Metrics

The application includes detailed performance tracking for:
- API request latency
- WebSocket message processing time
- Order placement timing
- Authentication timing

These metrics are recorded in the log files with the `[LATENCY]` tag.

## Testing Environment

By default, the application connects to the Deribit test environment:
- REST API: `https://test.deribit.com/api/v2/`
- WebSocket: `wss://test.deribit.com/ws/api/v2`

To use the production environment, modify the URLs in the respective source files.

## Project Structure

- `main.cpp` - Entry point and CLI interface
- `trader.hpp/cpp` - REST API client implementation
- `websocket.hpp/cpp` - WebSocket client for real-time data
- `logger.hpp/cpp` - Logging system implementation

## Notes

- This application is designed for the Deribit API v2
- The WebSocket client uses TLS for secure connections
- All operations are logged for audit and debugging purposes

