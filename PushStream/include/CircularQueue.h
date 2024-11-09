#include <vector>
#include <stdexcept>

template <typename T>
class CircularQueue {
public:
    explicit CircularQueue(size_t capacity)
        : buffer_(capacity), capacity_(capacity), head_(0), tail_(0), full_(false) {}

    // 添加元素，若队列已满则覆盖最旧的元素
    void push(const T& item) {
        buffer_[tail_] = item;
        tail_ = (tail_ + 1) % capacity_;

        if (full_) {
            head_ = (head_ + 1) % capacity_;  // 覆盖最旧的元素
        }

        full_ = (tail_ == head_);
    }

    // 弹出元素，若队列为空则抛出异常
    T pop() {
        if (empty()) {
            throw std::runtime_error("Queue is empty");
        }

        T item = buffer_[head_];
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
};
