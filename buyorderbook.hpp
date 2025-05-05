#ifndef BUYORDERBOOK_HPP
#define BUYORDERBOOK_HPP

#include <iostream>           // Input and output streams
#include <iomanip>            // Manipulators for I/O
#include <fstream>            // File I/O operations
#include <sstream>            // String stream operations
#include <string>             // String class
#include <vector>             // Dynamic array
#include <array>              // Static array
#include <deque>              // Double-ended queue
#include <list>               // Doubly linked list
#include <forward_list>       // Singly linked list
#include <stack>              // Stack container
#include <queue>              // Queue and priority queue
#include <map>                // Ordered map
#include <unordered_map>      // Unordered map (hash table)
#include <set>                // Ordered set
#include <unordered_set>      // Unordered set (hash table)
#include <algorithm>          // Algorithms (sorting, searching, etc.)
#include <functional>         // Function objects
#include <numeric>            // Numeric operations (accumulate, inner_product, etc.)
#include <cmath>              // Math functions
#include <limits>             // Numeric limits
#include <random>             // Random number generation
#include <ctime>              // Time functions
#include <chrono>             // Chrono library (time measurement)
#include <thread>             // Multithreading
#include <mutex>              // Mutex for synchronization
#include <future>             // Async and futures
#include <atomic>             // Atomic operations
#include <condition_variable> // Condition variable
#include <tuple>              // Tuples
#include <utility>            // Utility functions (pair, move, swap)
#include <memory>             // Smart pointers (unique_ptr, shared_ptr)
#include <new>                // Low-level memory management
#include <exception>          // Exception handling
#include <stdexcept>          // Standard exceptions
#include <cassert>            // Debugging assertions
#include <type_traits>        // Type traits
#include <cctype>             // Character classification
#include <cstring>            // C-style string functions
#include <cstdio>             // C-style I/O functions
#include <cstdlib>            // C-style utilities
#include <climits>            // Limits of integral types
#include <cfloat>             // Limits of floating-point types
#include <cstdint>            // Fixed-width integer types
#include <csignal>            // Signal handling
#include <bitset>             // Bitset class for bit manipulation
#include <complex>            // Complex numbers
#include <regex>              // Regular expressions

#include "io.hpp"
#include "sellorderbook.hpp"

struct BuyOrderbook
{
public:
    void addOrder(int id, int price, int cnt);
    int matchOrders(int id, int price, int cnt, int t);
    bool cancelOrder(int id);

private:
    std::map<int, std::list<Order>> buyOrders;
};

#endif