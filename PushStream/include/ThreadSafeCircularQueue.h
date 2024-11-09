#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeCircularQueue {
public:
    explicit ThreadSafeCircularQueue(size_t capacity) 
        : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), full_(false) {}

    // 添加元素
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        // 如果满了，覆盖最旧的数据
        if (full_) {
            head_ = (head_ + 1) % capacity_;
        }

        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;

        full_ = tail_ == head_;

        cv_.notify_one();  // 通知消费者有新数据
    }

    // 弹出元素，若队列为空则等待
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this] { return !empty(); });  // 等待直到队列非空

        auto item = buffer_[head_];
        head_ = (head_ + 1) % capacity_;
        full_ = false;

        return item;
    }

    // 检查队列是否为空
    bool empty() const {
        return (!full_ && (head_ == tail_));
    }

    // 检查队列是否已满
    bool full() const {
        return full_;
    }

    // 获取队列中的元素数量
    size_t size() const {
        if (full_) {
            return capacity_;
        }
        if (tail_ >= head_) {
            return tail_ - head_;
        }
        return capacity_ + tail_ - head_;
    }

    // 获取队列的容量
    size_t capacity() const {
        return capacity_;
    }

private:
    std::vector<T> buffer_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    bool full_;

    mutable std::mutex mutex_;            // 互斥锁
    std::condition_variable cv_;          // 条件变量用于等待和通知
};