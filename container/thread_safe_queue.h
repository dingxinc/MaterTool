﻿#pragma once
// 线程安全的队列
// 2024-6-22

#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>

template <typename T>
class threadsafe_queue
{
private:
    mutable std::mutex _mutex; /* mutable 表示即使有 const 限定，也可以对值进行修改 */
    std::queue<T> _queue;
    std::condition_variable _cond;

public:
    threadsafe_queue() {}
    threadsafe_queue(threadsafe_queue const& other)
    {                                                 /* 当没有声明移动构造时，const& 可以被移动构造调用 */
        std::lock_guard<std::mutex> lk(other._mutex); /* 加锁 */
        _queue = other._queue;
    }

    void push(T value)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        _queue.push(value);
        _cond.notify_one(); /* 其他线程消费队列的时候发现队列为空挂起了，我们 push 完队列有数据了，通知挂起的线程可以消费了 */
    }

    /* 阻塞的 pop */
    void wait_and_pop(T& value) /* 传递引用外面的值同样会被修改 */
    {                           /* 当队列为空的时候会等待，当队列不为空的时候才 pop */
        std::unique_lock<std::mutex> lk(_mutex);
        _cond.wait(lk, [this]
            { return !_queue.empty(); }); /* this 捕获这个类，可以使用你这个类所有成员和函数 当这个谓词返回 false 的时候，wait 就会卡在这里等待其他线程唤醒，队列为空，就在这里挂起 */
        /* 当其他线程 notify 后，队列非空 */
        value = _queue.front(); /* 因为 value 是 T 类型的引用，只在这里发生一次拷贝 */
        _queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(_mutex);
        _cond.wait(lk, [this]
            { return !_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
        _queue.pop();
        return res; /* 返回一个局部的指针或引用是危险的，但是智能指针不会有这个问题 */
    }

    /* 非阻塞的 pop */
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_queue.empty())
            return false; /* 队列为空，直接返回，非阻塞方式效率高 */
        value = _queue.front();
        _queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(_mutex);
        if (_queue.empty())
            return std::make_shared<T>(); /* 队列为空返回空指针 */
        std::shared_ptr<T> res(std::make_shared<T>(_queue.front()));
        _queue.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(_mutex);
        return _queue.empty();
    }
};

/* 上面的队列面临线程执行wait_and_pop时如果出现了异常，导致数据被滞留在队列中，其他线程也无法被唤醒的情况。可以使用智能指针解决 */
template <typename T>
class threadsafe_queue_ptr
{
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T>> data_queue;
    std::condition_variable data_cond;

public:
    threadsafe_queue_ptr()
    {
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]
            { return !data_queue.empty(); });
        value = std::move(*data_queue.front()); // ⇽-- - 1
        data_queue.pop();
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(*data_queue.front()); // ⇽-- - 2
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]
            { return !data_queue.empty(); });
        std::shared_ptr<T> res = data_queue.front(); // ⇽-- - 3
        data_queue.pop();
        return res;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res = data_queue.front(); // ⇽-- - 4
        data_queue.pop();
        return res;
    }

    void push(T new_value)
    {
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value))); // ⇽-- - 5
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};