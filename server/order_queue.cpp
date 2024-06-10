#include "order_queue.hpp"

OrderQueue::OrderQueue() {
}

bool OrderQueue::push(const Serialize::TradeOrder& order) {
    return concurrent_queue_.enqueue(order);
}

bool OrderQueue::pop(Serialize::TradeOrder& order) {
    return concurrent_queue_.try_dequeue(order);
}
