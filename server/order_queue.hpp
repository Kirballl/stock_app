#ifndef ORDER_QUEUE
#define ORDER_QUEUE

#include <thread>
#include <vector>
#include <memory>
#include <mutex>

#include "trade_market_protocol.pb.h"
#include "concurrentqueue.h" 

class OrderQueue {
public:
    OrderQueue();
    bool push(const Serialize::TradeOrder& order);
    bool pop(Serialize::TradeOrder& order);
    bool is_empty() const;

private:
    moodycamel::ConcurrentQueue<Serialize::TradeOrder> concurrent_queue_;
};

#endif // ORDER_QUEUE
