#pragma once
// 可中断线程
// 2022-6-22
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <future>

extern void interruption_point();

class thread_interrupted : public std::exception
{
public:
    thread_interrupted() : message("thread interrupted.") {}
    ~thread_interrupted() throw () {
    }

    virtual const char* what() const throw () {
        return message.c_str();
    }

private:
    std::string message;
};

// 中断标记
class interrupt_flag
{
    std::atomic<bool> flag;
    std::condition_variable* thread_cond;
    std::condition_variable_any* thread_cond_any;
    std::mutex set_clear_mutex;
public:
    interrupt_flag() :
        thread_cond(0), thread_cond_any(0)
    {}

    void set()
    {
        flag.store(true, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        if (thread_cond)
        {
            thread_cond->notify_all();
        }
        else if (thread_cond_any) {
            thread_cond_any->notify_all();
        }
    }

    bool is_set() const
    {
        return flag.load(std::memory_order_relaxed);
    }

    // 设置flag关联的条件变量，因为需要用指定的条件变量通知挂起的线程
    void set_condition_variable(std::condition_variable& cv)
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = &cv;
    }

    // 清除关联的条件变量
    void clear_condition_variable()
    {
        std::lock_guard<std::mutex> lk(set_clear_mutex);
        thread_cond = 0;
    }

    // wait操作封装了接受任意锁的等待操作，wait函数内部定义了custom_lock，封装了加锁，解锁等操作
    // 内部构造了custom_lock对象 cl 主要是对 set_clear_mutex 加锁，然后在调用 cv.wait，这样能和 set
    // 函数中的通知条件变量构成互斥，这么做的好处就是要么先将 flag 设置为 true 并发送通知，要么先 wait，然后再发送通知。
    // 这样避免了线程在 wait 处卡死(线程不会错过发送的通知)
    template<typename Lockable>
    void wait(std::condition_variable_any& cv, Lockable& lk) {
        struct custom_lock {
            interrupt_flag* self;
            Lockable& lk;
            custom_lock(interrupt_flag* self_, std::condition_variable_any& cond, Lockable& lk_) :
                self(self_), lk(lk_) {
                self->set_clear_mutex.lock();
                self->thread_cond_any = &cond;
            }

            void unlock() {
                lk.unlock();
                self->set_clear_mutex.unlock();
            }

            void lock() {
                std::lock(self->set_clear_mutex, lk);
            }

            ~custom_lock() {
                self->thread_cond_any = 0;
                self->set_clear_mutex.unlock();
            }
        };

        custom_lock cl(this, cv, lk);
        interruption_point();
        cv.wait(cl);
        interruption_point();
    }
};

extern thread_local interrupt_flag this_thread_interrupt_flag;

// 这个类主要是用来在析构时释放和flag关联的条件变量
struct clear_cv_on_destruct {
    ~clear_cv_on_destruct();
};

class interruptible_thread
{
private:
    std::thread internal_thread;
    interrupt_flag* flag;
public:
    template<typename FunctionType>
    interruptible_thread(FunctionType f)
    {
        //⇽-- - 2
        std::promise<interrupt_flag*> p;
        //⇽-- - 3
        internal_thread = std::thread([f, &p] {
            p.set_value(&this_thread_interrupt_flag);
            //⇽-- - 4
            f();
            });
        //⇽-- - 5
        flag = p.get_future().get();
    }

    void join() {
        internal_thread.join();
    }
    void interrupt()
    {
        if (flag)
        {
            //⇽-- - 6
            flag->set();
        }
    }
};

// 支持普通条件变量的等待
extern void interruptible_wait(std::condition_variable& cv,
    std::unique_lock<std::mutex>& lk);

// 支持谓词的等待
template<typename Predicate>
void interruptible_wait(std::condition_variable& cv,
    std::unique_lock<std::mutex>& lk,
    Predicate pred)
{
    interruption_point();
    this_thread_interrupt_flag.set_condition_variable(cv);
    clear_cv_on_destruct guard;
    while (!this_thread_interrupt_flag.is_set() && !pred())
    {
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    interruption_point();
}

template<typename Lockable>
void interruptible_wait(std::condition_variable_any& cv, Lockable& lk) {
    this_thread_interrupt_flag.wait(cv, lk);
}

// 支持future的等待
template<typename T>
void interruptible_wait(std::future<T>& uf)
{
    while (!this_thread_interrupt_flag.is_set())
    {
        if (uf.wait_for(std::chrono::milliseconds(1)) ==
            std::future_status::ready)
            break;
    }
    interruption_point();
}