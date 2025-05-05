#include <iostream>			  // Input and output streams
#include <iomanip>			  // Manipulators for I/O
#include <fstream>			  // File I/O operations
#include <sstream>			  // String stream operations
#include <string>			  // String class
#include <vector>			  // Dynamic array
#include <array>			  // Static array
#include <deque>			  // Double-ended queue
#include <list>				  // Doubly linked list
#include <forward_list>		  // Singly linked list
#include <stack>			  // Stack container
#include <queue>			  // Queue and priority queue
#include <map>				  // Ordered map
#include <unordered_map>	  // Unordered map (hash table)
#include <set>				  // Ordered set
#include <unordered_set>	  // Unordered set (hash table)
#include <algorithm>		  // Algorithms (sorting, searching, etc.)
#include <functional>		  // Function objects
#include <numeric>			  // Numeric operations (accumulate, inner_product, etc.)
#include <cmath>			  // Math functions
#include <limits>			  // Numeric limits
#include <random>			  // Random number generation
#include <ctime>			  // Time functions
#include <chrono>			  // Chrono library (time measurement)
#include <thread>			  // Multithreading
#include <mutex>			  // Mutex for synchronization
#include <future>			  // Async and futures
#include <atomic>			  // Atomic operations
#include <condition_variable> // Condition variable
#include <tuple>			  // Tuples
#include <utility>			  // Utility functions (pair, move, swap)
#include <memory>			  // Smart pointers (unique_ptr, shared_ptr)
#include <new>				  // Low-level memory management
#include <exception>		  // Exception handling
#include <stdexcept>		  // Standard exceptions
#include <cassert>			  // Debugging assertions
#include <type_traits>		  // Type traits
#include <cctype>			  // Character classification
#include <cstring>			  // C-style string functions
#include <cstdio>			  // C-style I/O functions
#include <cstdlib>			  // C-style utilities
#include <climits>			  // Limits of integral types
#include <cfloat>			  // Limits of floating-point types
#include <cstdint>			  // Fixed-width integer types
#include <csignal>			  // Signal handling
#include <bitset>			  // Bitset class for bit manipulation
#include <complex>			  // Complex numbers
#include <regex>			  // Regular expressions

#include "io.hpp"
#include "sellorderbook.hpp"
#include "buyorderbook.hpp"
#include "engine.hpp"

void Engine::accept(ClientConnection connection)
{
	auto thread = std::thread(&Engine::connection_thread, this, std::move(connection));
	thread.detach();
}

int Engine::getCurrentTimestamp()
{
	return currTime.fetch_add(1, std::memory_order_acq_rel);
}

void Engine::handleCancelOrder(int id, std::unordered_map<int, ClientOrderRecord> &clientsOrders)
{
	if (!clientsOrders.count(id))
	{
		Output::OrderDeleted(id, false, getCurrentTimestamp());
		return;
	}
	else
	{
		ClientOrderRecord rec = clientsOrders[id];

		bool isSuccess = false;
		{
			std::unique_lock<std::mutex> lock1(globalMut);

			std::shared_ptr<std::mutex> mtx = instrumentToBooks[rec.instrument].second;
			std::unique_lock<std::mutex> lock2(*mtx);
			if (rec.isSell)
			{
				std::shared_ptr<SellOrderbook> sb = instrumentToBooks[rec.instrument].first.first;
				isSuccess = sb->cancelOrder(id);
			}
			else
			{
				std::shared_ptr<BuyOrderbook> bb = instrumentToBooks[rec.instrument].first.second;
				isSuccess = bb->cancelOrder(id);
			}
		}

		if (isSuccess)
			Output::OrderDeleted(id, true, getCurrentTimestamp());
		else
			Output::OrderDeleted(id, false, getCurrentTimestamp());

		clientsOrders.erase(id);
	}
}

bool Engine::handleBuyOrder(std::string instrument, int id, int price, int cnt)
{
	// Instrument is New
	bool isNew = false;
	{
		std::unique_lock<std::mutex> lock(globalMut);
		if (!instrumentToBooks.count(instrument))
		{
			auto sb = std::make_shared<SellOrderbook>();
			auto bb = std::make_shared<BuyOrderbook>();
			std::shared_ptr<std::mutex> mtx = std::make_shared<std::mutex>();

			bb->addOrder(id, price, cnt);

			instrumentToBooks.emplace(instrument,
									  std::make_pair(std::make_pair(sb, bb), mtx));

			Output::OrderAdded(id, &instrument[0], price, cnt, false, getCurrentTimestamp());

			isNew = true;
		}
	}

	if (isNew)
		return false;

	// Obtain Instrument's SellOrderbook and BuyOrderbook
	std::shared_ptr<SellOrderbook> sb;
	std::shared_ptr<BuyOrderbook> bb;
	std::shared_ptr<std::mutex> mtx;
	{
		std::unique_lock lock(globalMut);
		std::pair<std::shared_ptr<SellOrderbook>, std::shared_ptr<BuyOrderbook>> b = instrumentToBooks[instrument].first;
		sb = b.first;
		bb = b.second;
		mtx = instrumentToBooks[instrument].second;
	}

	// Find a Match in SellOrderbook
	std::unique_lock lock(*mtx);
	int amtSold = sb->matchOrders(id, price, cnt, getCurrentTimestamp());
	cnt -= amtSold;

	// Postprocessing - Fulfilled vs Not Fulfilled
	if (cnt == 0)
	{
		return true;
	}
	else
	{
		bb->addOrder(id, price, cnt);
		Output::OrderAdded(id, &instrument[0], price, cnt, false, getCurrentTimestamp());
		return false;
	}
}

bool Engine::handleSellOrder(std::string instrument, int id, int price, int cnt)
{
	// Instrument is New
	bool isNew = false;
	{
		std::unique_lock<std::mutex> lock(globalMut);
		if (!instrumentToBooks.count(instrument))
		{
			auto sb = std::make_shared<SellOrderbook>();
			auto bb = std::make_shared<BuyOrderbook>();
			std::shared_ptr<std::mutex> mtx = std::make_shared<std::mutex>();

			sb->addOrder(id, price, cnt);

			instrumentToBooks.emplace(instrument,
									  std::make_pair(std::make_pair(sb, bb), mtx));

			Output::OrderAdded(id, &instrument[0], price, cnt, true, getCurrentTimestamp());

			isNew = true;
		}
	}

	if (isNew)
		return false;

	// Obtain Instrument's SellOrderbook and BuyOrderbook
	std::shared_ptr<SellOrderbook> sb;
	std::shared_ptr<BuyOrderbook> bb;
	std::shared_ptr<std::mutex> mtx;
	{
		std::unique_lock lock(globalMut);
		std::pair<std::shared_ptr<SellOrderbook>, std::shared_ptr<BuyOrderbook>> b = instrumentToBooks[instrument].first;
		sb = b.first;
		bb = b.second;
		mtx = instrumentToBooks[instrument].second;
	}

	// Find a Match in BuyOrderbook
	std::unique_lock lock(*mtx);
	int amtBought = bb->matchOrders(id, price, cnt, getCurrentTimestamp());
	cnt -= amtBought;

	// Postprocessing - Fulfilled vs Not Fulfilled
	if (cnt == 0)
	{
		return true;
	}
	else
	{
		sb->addOrder(id, price, cnt);
		Output::OrderAdded(id, &instrument[0], price, cnt, true, getCurrentTimestamp());
		return false;
	}
}

void Engine::connection_thread(ClientConnection connection)
{
	std::unordered_map<int, ClientOrderRecord> clientsOrders;
	while (true)
	{
		ClientCommand input{};
		switch (connection.readInput(input))
		{
		case ReadResult::Error:
			SyncCerr{} << "Error reading input" << std::endl;
		case ReadResult::EndOfFile:
			return;
		case ReadResult::Success:
			break;
		}

		std::string instrument = input.instrument;
		int id = input.order_id;
		int price = input.price;
		int cnt = input.count;

		switch (input.type)
		{
		case input_cancel:
		{
			SyncCerr{} << "Got cancel: ID: " << input.order_id << std::endl;

			handleCancelOrder(id, clientsOrders);
			break;
		}

		case input_buy:
		{
			SyncCerr{} << "Got order: B " << input.instrument << " x " << input.count << " @ " << input.price << " ID: " << input.order_id << std::endl;

			bool isSuccess = handleBuyOrder(instrument, id, price, cnt);

			if (!isSuccess)
			{
				clientsOrders[id] = ClientOrderRecord{instrument, false};
			}

			break;
		}

		case input_sell:
		{
			SyncCerr{} << "Got order: S " << input.instrument << " x " << input.count << " @ " << input.price << " ID: " << input.order_id << std::endl;

			bool isSuccess = handleSellOrder(instrument, id, price, cnt);

			if (!isSuccess)
			{
				clientsOrders[id] = ClientOrderRecord{instrument, true};
			}

			break;
		}

		default:
		{
			// SyncCerr{}
			// 	<< "Got order: " << static_cast<char>(input.type) << " " << input.instrument << " x " << input.count << " @ "
			// 	<< input.price << " ID: " << input.order_id << std::endl;

			// auto output_time = getCurrentTimestamp();
			// Output::OrderAdded(input.order_id, input.instrument, input.price, input.count, input.type == input_sell,
			// 				   output_time);
			break;
		}

			// Additionally:

			// Remember to take timestamp at the appropriate time, or compute
			// an appropriate timestamp!
			intmax_t output_time = getCurrentTimestamp();

			// Check the parameter names in `io.hpp`.
			Output::OrderExecuted(123, 124, 1, 2000, 10, output_time);
		}
	}
}