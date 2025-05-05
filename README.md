# Exchange Matching Engine Writeup

This C++ program simulates a matching engine inside an exchange that allows the concurrent matching of buy and sell orders. When an exchange receives a new order, it is considered ‘active’, and it will first try to match it against ‘resting’ orders. In case the exchange cannot match the active order against any resting orders, it will store the active order in a resting order book, so that it can potentially match it later.

This matching engine follows the price-time priority rule. The following invariants must hold for any matching to happen:

- The side of the two orders must be different (i.e. a buy order must match against a sell orders or vice versa).

- The instrument of the two orders must be the same (i.e. an order for “GOOG” must match against another order for “GOOG”).

- The size of the two orders must be greater than zero.

- The price of the buy order must be greater or equal to the price of the sell order.

- In case an active order can be matched with multiple resting orders, the resting order with the best price is matched first. Intuitively, as a buyer, you want to buy for the cheapest amount; as a seller, you want to sell for the most money. So, an active buy order matches with the lowest-priced resting sell order, while an active sell order matches with the highest-priced resting buy order.

## Example Inputs and Outputs

### Inputs

1. Buy Order : `B <Order ID> <Instrument> <Price> <Count>`
2. Sell Order : `S <Order ID> <Instrument> <Price> <Count>`
3. Cancel Order : `C <Order ID>`

### Outputs

1. Order Added to Resting Order Book : `<B/S> <Order ID> <Instrument> <Price> <Count> <Timestamp Completed>`
2. Order Executed : `E <Resting Order ID> <Active Order ID> <Execution ID> <Price> <Count> <Timestamp Completed>`
3. Order Cancelled : `X <Order ID> <A/R> <Timestamp Completed>`

## Description of Data Structures

1. Instrument Order Mapping:

   A HashMap (`unordered_map`) is used to efficiently map each financial instrument to its corresponding resting sell and buy orders.

2. Price-Level Ordering:

   For each instrument, sell and buy orders are stored separately in an Ordered HashMap (`map`), which maintains the orders sorted by price.

3. Order Storage at Each Price Level:

   At a given price level, orders are stored in a Linked List (`list`). This allows efficient insertion, deletion, and sequential traversal while preserving order priority (FIFO execution within the same price level).

### Synchronisation Primitives:

- `currTime` is an atomic variable.
- `instrumentToBooks` is protected by a mutex, `globalMut`
  - This prevents any data races when reading from and writing to it but allows for different instruments to access their resting books concurrently.
- Both resting order books, `SellOrderbook` and `BuyOrderbook` are protected by a single shared mutex.
  - This prevents any data races within either resting order book.

## Explanation of Concurrency

This Exchange Matching Engine supports Instrument-Level Concurrency.

- Orders for different instruments can execute concurrently.

### Concurrent Processing Across Instruments

Since each instrument has independent resting sell and buy order books, orders for different instruments can be processed concurrently without interference.

- `instrumentToBooks[instrument] = {SellOrderbookAndMutex, BuyOrderbookAndMutex}`

An instrument locks `globalMut` before accessing `instrumentToBooks` in either of 2 cases:

1. Inserting a new `instrument` into `intrumentToBooks`
2. Obtaining the `instrument`'s resting sell orders and resting buy orders

Moreover, the locking of `globalMut` is kept to as small a scope as possible, only executing either of the above 2 scenarios mentioned and then releasing the lock.

## Description of Tests

Testing was done incrementally. Firstly, basic functionalities were tested using single-threaded testcases. Secondly, manual testing of multi-threaded scenarios were done.

Lastly, to stress test the entire program, there are 15 custom testcases. Each custom generated testcase consists of 40 threads each, performing 100 operations each.