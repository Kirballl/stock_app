# Stock trading application

![Logo](stock_logo.io.png)

## Overview 

This is a simple stock exchange application with a client-server architecture with authentication. The exchange trades USD for RUB, matching buy and sell orders from multiple clients.

##  Features 

- 👥 **Multi-client support**: The server handles multiple client connections simultaneously.
- 📝 **Order placement**: Clients can place buy or sell orders for USD/RUB.
- 💰 **Balance checking**: Clients can view their current balance.
- 🤝 **Automatic order matching**: The server automatically matches orders when prices intersect.
- ⚖️ **Partial order execution**: Orders can be partially filled.
- ⏱️ **Real-time trading**: Orders are active until fully executed.
- 🚫 **Unlimited balance**: Clients can trade with negative balances.
- 📋 **Active order viewing**: Clients can see their current active orders.
- 📜 **Trade history**: View completed trades.
- 📈 **Quote history**: Access historical price quotes.
- ❌ **Order cancellation**: Ability to cancel active orders.
- 🗄️ **Database integration**: PostgreSQL used for storing order and trade history.
- 🔒 **Secure authentication**: Client authentication with password protection.

## Prerequisites 

- CMake (version 3.21 or higher)
- C++17 compatible compiler
- Docker and Docker Compose
- Boost (version 1.40 or higher)
- Protobuf
- PostgreSQL client libraries
- GTest

## Building the Project

### 📎 Initialize and update submodules
```bash
git submodule update --init --recursive
```

### 🏗️ Create dir and build all
```bash
mkdir build && cd build
cmake .. --target all
```

### 🐳  Run database
```bash
docker compose up --build -d
```
### 🖥️  Run server
```bash
./build/server/server
```

### 🖥️  Run client
```bash
./build/client/client
```

### 🧪 Run tests
```bash
./build/tests/trade_tests
```
## Usage Instructions

After launching the client application, you will be presented with the following options:

### 🔐 Authentication Menu:
1. **Sign up**: ✍️ Create a new account
2. **Sign in**: 🔑 Log into an existing account
3. **Exit**:🚪 Close the application

### 📊 Main Menu:
1. **📝Make order**: Place a new buy or sell order
   - Choose order type (buy/sell)
   - Enter USD cost in RUB
   - Enter USD amount
2. **💰View my balance**: Check your current account balance
3. **📋View all active orders**: Display all your active orders
4. **📜View last completed trades**: Show recent completed trades
5. **📈View quote history**: Display historical price quotes
6. **❌Cancel active order**: Cancel an existing order
   - Choose order type (buy/sell)
   - Enter the order ID you wish to cancel
7. **🚪Exit**: Log out and close the application

### 💡 Tips:
- When entering numeric values, follow the prompts for valid ranges.
- For cancelling orders, you may want to view active orders first to get the correct order ID.

## Project Architecture

The Stock Trading Application is built with a client-server architecture:

### Client Side:
- 🔌`client.cpp/hpp`: Handles client-side logic and communication with the server.
- 💻`user_interface.cpp/hpp`: Manages the console-based user interface.

### Server Side:
- 🔐`auth.cpp/hpp`: generate and verify jwt.
- 💾`client_data_manager.cpp/hpp`: Manages in-memory client data and orders info.
  - Handles client account balances and order history in RAM during server runtime
  - Loads data from database on server startup and persists to database on shutdown
  - Processes active, completed, and cancelled orders
  - Provides interfaces for updating client data and order status
  - Ensures high-performance data access and manipulation during server operation
- 🧠`core.cpp/hpp`: Core business logic for order matching and trade execution.
- 🗄️`database.cpp/hpp`: Database interactions.
- 📊`order_queue.cpp/hpp`: Wrapper over concurrentqueue.h.
- 🖥️`server.cpp/hpp`: Server logic.
- 🔌`session_client_connection.cpp/hpp`: Managing a certain client connection.
- 👥`session_manager.cpp/hpp`: Manages client sessions and orchestrates communication between server components.
  - Handles client connections and disconnections
  - Initializes and manages core server components
  - Coordinates client data and maintains session state
  - Controls server lifecycle and ensures thread-safe operations
- ⏱️`time_order_utils.cpp/hpp`: Generate order id and get current timestamp.

### Common Components:
- 📡`proto/trade_market_protocol.proto`: Defines the protocol buffer messages for client-server communication.
- ⚙️`config/`: Contains configuration files for the server, database and jwt.
- 🔨`common/`: Shared utilities and definitions.

### Third-patry:
- 🔒`bcrypt` : A library to hash passwords.
- 🎟️`jwt-cpp`: A library for creating and validating JSON Web Tokens.
- 🔄`moodycamel::concurrentqueue` : An industrial-strength lock-free queue.
- 📝`jwt-cpp`: A fast logging library.

### Testing:
- 🔬`tests/`: Contains unit tests for core components.

The application uses Protocol Buffers for serialization, PostgreSQL for data storage, and CMake for build management.