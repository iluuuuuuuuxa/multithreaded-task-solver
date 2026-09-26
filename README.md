# Multithreaded Task Execution Engine

## Overview
This project implements a highly concurrent execution engine in C++, designed to efficiently distribute heavy computational workloads across multiple CPU cores. The system processes asynchronous data requests and dispatches them to a dynamically managed pool of worker threads, ensuring optimal performance and resource utilization.

## Architecture & Core Features
*   **Multithreaded Dispatching:** Utilizes C++ standard threads (or POSIX threads) to manage a central worker pool, handling incoming requests concurrently.
*   **Producer-Consumer Pattern:** Implements a robust producer-consumer architecture. Customer support threads listen for incoming calculation requests and feed them into a shared task queue, while worker threads consume and process these tasks.
*   **Thread Synchronization:** Engineered strict synchronization mechanisms using `std::mutex` and `std::condition_variable` to prevent race conditions, data corruption, and deadlocks during shared memory access.
*   **Asynchronous Data Delivery:** Supports both synchronous and asynchronous data fetching from external providers (simulated as data producers), requiring complex state tracking to ensure tasks are only executed when all dependencies are met.
*   **Reentrant Callbacks:** Features thread-safe, reentrant callback methods for returning processed data to the requesters without bottlenecking the system.

## Technical Stack
* **Language:** C++
* **Concurrency:** `std::thread`, `std::mutex`, `std::condition_variable`, POSIX API
* **Concepts:** Concurrency, Shared Memory Management, Deadlock Prevention, Load Balancing

## How to Build and Run

The project includes a `Makefile` for straightforward compilation. 

1. Build the project using `make`: make
2. Execute the compiled binary: ./test

## Disclaimer
*This project was developed as part of the Operating Systems course at the Faculty of Information Technology, CTU in Prague. The code demonstrates advanced multithreading concepts and synchronization primitives.*

## Note on Compilation
This project relies on a proprietary static testing library (*libprogtest_solver.a*) provided by the CTU evaluation platform, which is deliberately excluded from this repository to comply with Git best practices and university copyright. The source code (*solution.cpp*) is provided here primarily for demonstration, architectural evaluation, and code review.
