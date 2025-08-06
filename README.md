# NASDAQ ITCH 5.0 Parser in C++

This project is a high-performance C++ parser for the NASDAQ TotalView-ITCH 5.0 market data feed protocol. It's designed to process real-time financial data efficiently, featuring a multi-threaded architecture to handle redundant data streams.

---

## Features

* **ITCH 5.0 Protocol Support:** Parses the 8 core message types that define an order's lifecycle (System Event, Stock Directory, Add Order, Execute, Cancel, Delete, Replace).
* **Multi-Threaded Architecture:** Uses C++ `std::thread` to listen to two separate network feeds concurrently.
* **A/B Feed Arbitration:** Implements a producer-consumer pattern with a thread-safe queue (`std::queue` & `std::mutex`) to process data from two redundant feeds.
* **Duplicate Rejection:** The arbitration logic correctly identifies and discards duplicate messages based on their tracking number, ensuring each message is processed only once.
* **Clean Code:** The project is structured with separate header and implementation files for modularity and readability.

---

## How to Build

The project uses CMake for building.

1.  **Navigate to the project directory:**
    ```bash
    cd nasdaq-parser
    ```
2.  **Create and enter the build directory:**
    ```bash
    mkdir build && cd build
    ```
3.  **Run CMake and Make:**
    ```bash
    cmake ..
    make
    ```
    The executable `nasdaq_parser` will be created inside the `build` directory.

---

## How to Run

The application requires a data feed simulator to run. A simple Python sender is included for this purpose. You will need **two separate terminals**.

**Terminal 1: Run the C++ Parser**
```bash
# Navigate to the build directory
cd nasdaq-parser/build

# Run the parser (it will wait for data)
./nasdaq_parser
