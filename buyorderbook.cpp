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

#include "buyorderbook.hpp"
#include "engine.hpp"
#include "io.hpp"

void BuyOrderbook::addOrder(int id, int price, int cnt) { buyOrders[price].push_back(Order{id, price, cnt, 1}); }

int BuyOrderbook::matchOrders(int id, int price, int cnt, int t)
{
    int amtBought = 0;

    for (auto it = buyOrders.rbegin(); it != buyOrders.rend() and cnt > 0;)
    {
        if (it->first < price)
            break;

        auto &lst = it->second;
        for (auto lstIt = lst.begin(); lstIt != lst.end() and cnt > 0;)
        {
            int buy = std::min(lstIt->quantity, cnt);

            lstIt->quantity -= buy;
            cnt -= buy;
            amtBought += buy;

            Output::OrderExecuted(lstIt->id, id, lstIt->e, lstIt->price, buy, t);

            lstIt->e += 1;

            if (lstIt->quantity == 0)
                lstIt = lst.erase(lstIt);
            else
                lstIt++;
        }

        if (buyOrders.empty())
        {
            auto normalIt = it.base();
            normalIt--;

            it = make_reverse_iterator(buyOrders.erase(normalIt));
        }
        else
        {
            it++;
        }
    }

    return amtBought;
}

bool BuyOrderbook::cancelOrder(int id)
{
    for (auto it = buyOrders.begin(); it != buyOrders.end(); it++)
    {
        auto &lst = it->second;
        for (auto lstIt = lst.begin(); lstIt != lst.end(); lstIt++)
        {
            if (lstIt->id == id)
            {

                lst.erase(lstIt);
                if (lst.empty())
                {
                    buyOrders.erase(it);
                }
                return true;
            }
        }
    }

    return false;
}
