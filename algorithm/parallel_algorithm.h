#pragma once
// stl 常用算法的并行实现
// 2024-6-22

#include "../thread/join_threads.h"
#include <future>
#include <atomic>
#include <numeric>

template <typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f) {
	/* 1. 计算容器元素的个数 */
	unsigned long const length = std::distance(first, last);
	if (!length)
		return;

	/* 2. 每 25 个元素开一个线程计算 */
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;

	/* 3. 获取硬件线程数目 */
	unsigned long const hardware_threads = std::thread::hardware_concurrency();

	/* 4. 计算开辟线程数量 */
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	unsigned long const block_size = length / num_threads;  // 分块

	/* 5. 主线程需要汇总主线程的结果 */
	std::vector<std::future<void>> futures(num_threads - 1);   //⇽-- - 1
	std::vector<std::thread> threads(num_threads - 1);
	join_threads joiner(threads);

	/* 6. 开始进行并行计算 */
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size); // 向前移动迭代器
		std::packaged_task<void(void)> task( // ⇽-- - 2
			[=]()
			{
				std::for_each(block_start, block_end, f);  // 调用 stl 中的 for_each 计算
			});
		futures[i] = task.get_future();    // 获取计算结果
		threads[i] = std::thread(std::move(task));    //⇽-- - 3
		block_start = block_end;
	}
	/* 7. 主线程计算最后一个块的数据 */
	std::for_each(block_start, last, f);
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		// 主线程获取到前几个线程的结果
		futures[i].get();   // ⇽-- - 4
	}
}

// 递归划分的方式实现 for_each
template<typename Iterator, typename Func>
void async_for_each(Iterator first, Iterator last, Func f)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return;
	unsigned long const min_per_thread = 25;
	if (length < (2 * min_per_thread))
	{
		std::for_each(first, last, f);    //⇽-- - 1
	}
	else
	{
		Iterator const mid_point = first + length / 2;
		//⇽-- - 2
		std::future<void> first_half = std::async(&async_for_each<Iterator, Func>,// 开辟本地线程
			first, mid_point, f);
		//⇽-- - 3
		async_for_each(mid_point, last, f);  // 主线程
		// ⇽-- - 4
		first_half.get();//汇总结果
	}
}

// 并行查找 find
template<typename Iterator, typename MatchType>  // MatchType 查找匹配的值
Iterator parallel_find(Iterator first, Iterator last, MatchType match)
{
    struct find_element    //⇽-- - 1
    {
        // 实现仿函数功能
        void operator()(Iterator begin, Iterator end,
            MatchType match,
            std::promise<Iterator>* result,
            std::atomic<bool>* done_flag)
        {
            try
            {// done_flag 表示是否已经查到，如果别的线程已经查到了，就直接返回
                for (; (begin != end) && !done_flag->load(); ++begin)    //⇽-- - 2
                {
                    if (*begin == match)
                    {
                        result->set_value(begin);    //⇽-- - 3
                        done_flag->store(true);    //⇽-- - 4
                        return;
                    }
                }
            }
            catch (...)    //⇽-- - 5
            {
                try
                {
                    result->set_exception(std::current_exception());    //⇽-- - 6
                    done_flag->store(true);
                }
                catch (...)    //⇽-- - 7
                {
                }
            }
        }
    };

    unsigned long const length = std::distance(first, last);
    if (!length)
        return last;

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::promise<Iterator> result;    //⇽-- - 8
    std::atomic<bool> done_flag(false);     //⇽-- - 9
    std::vector<std::thread> threads(num_threads - 1); //⇽-- - 10
    {
        join_threads joiner(threads);
        Iterator block_start = first;
        for (unsigned long i = 0; i < (num_threads - 1); ++i)
        {
            Iterator block_end = block_start;
            std::advance(block_end, block_size);
            // ⇽-- - 11
            threads[i] = std::thread(find_element(), block_start, block_end, match, &result, &done_flag);
            block_start = block_end;
        }
        // ⇽-- - 12
        find_element()(block_start, last, match, &result, &done_flag);
    }

    // ⇽-- - 13
    if (!done_flag.load())//没有查到
    {
        return last;
    }
    //⇽-- - 14
    return result.get_future().get();
}

// 递归折半查找
template<typename Iterator, typename MatchType>
Iterator parallel_find_impl(Iterator first, Iterator last, MatchType match,
    std::atomic<bool>& done)   // ⇽-- - 1
{
    try
    {
        unsigned long const length = std::distance(first, last);
        unsigned long const min_per_thread = 25;   // ⇽-- - 2
        if (length < (2 * min_per_thread))    //⇽-- - 3
        {
            for (; (first != last) && !done.load(); ++first)     //⇽-- - 4
            {
                if (*first == match)
                {
                    done = true;    //⇽-- - 5
                    return first;
                }
            }
            return last;    //⇽-- - 6
        }
        else
        {
            //⇽-- - 7
            Iterator const mid_point = first + (length / 2);
            //⇽-- - 8
            std::future<Iterator> async_result = std::async(&parallel_find_impl<Iterator, MatchType>,
                mid_point, last, match, std::ref(done));
            //⇽-- - 9
            Iterator const direct_result = parallel_find_impl(first, mid_point, match, done);
            //⇽-- - 10
            return (direct_result == mid_point) ? async_result.get() : direct_result;
        }
    }
    catch (...)
    {
        // ⇽-- - 11
        done = true;
        throw;
    }
}

template<typename Iterator, typename MatchType>
Iterator parallel_find_async(Iterator first, Iterator last, MatchType match)
{
    std::atomic<bool> done(false);
    //⇽-- - 12
    return parallel_find_impl(first, last, match, done);
}


// 并行局部求和
template<typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{
    typedef typename Iterator::value_type value_type;

    struct process_chunk    //⇽-- - 1
    {
        void operator()(Iterator begin, Iterator last,
            std::future<value_type>* previous_end_value,  // 每一个区间最后一个元素
            std::promise<value_type>* end_value)
        {
            try
            {
                Iterator end = last;
                ++end;
                std::partial_sum(begin, end, begin);    //⇽-- - 2
                if (previous_end_value)    //⇽-- - 3
                {
                    value_type addend = previous_end_value->get();   // ⇽-- - 4 串行
                    *last += addend;   // ⇽-- - 5
                    if (end_value)
                    {
                        end_value->set_value(*last);    //⇽-- - 6  // 并行
                    }
                    // ⇽-- - 7
                    std::for_each(begin, last, [addend](value_type& item)
                        {
                            item += addend;
                        });
                }
                else if (end_value)
                {
                    // ⇽-- - 8
                    end_value->set_value(*last);
                }
            }
            catch (...)  // ⇽-- - 9
            {
                if (end_value)
                {
                    end_value->set_exception(std::current_exception());   // ⇽-- - 10
                }
                else
                {
                    throw;   // ⇽-- - 11
                }

            }
        }
    };
    unsigned long const length = std::distance(first, last);

    if (!length) {
        return;
    }
    unsigned long const min_per_thread = 25;     //⇽-- - 12
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    unsigned long const block_size = length / num_threads;
    typedef typename Iterator::value_type value_type;

    std::vector<std::thread> threads(num_threads - 1);   // ⇽-- - 13

    std::vector<std::promise<value_type> > end_values(num_threads - 1);   // ⇽-- - 14

    std::vector<std::future<value_type> > previous_end_values;   // ⇽-- - 15
    previous_end_values.reserve(num_threads - 1);   // ⇽-- - 16
    join_threads joiner(threads);
    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_last = block_start;
        std::advance(block_last, block_size - 1);   // ⇽-- - 17
        // ⇽-- - 18
        threads[i] = std::thread(process_chunk(), block_start, block_last,
            (i != 0) ? &previous_end_values[i - 1] : 0,
            &end_values[i]);
        block_start = block_last;
        ++block_start;   // ⇽-- - 19
        previous_end_values.push_back(end_values[i].get_future());   // ⇽-- - 20
    }
    Iterator final_element = block_start;
    std::advance(final_element, std::distance(block_start, last) - 1);   // ⇽-- - 21
    // ⇽-- - 22
    process_chunk()(block_start, final_element, (num_threads > 1) ? &previous_end_values.back() : 0,
        0);

}
/*因为我们处理的区间不一定是首个区间，也就是他还需要加上前面区间处理得出的最后一个元素的值，所以我们通过previouse_end_value判断本区间不是首个区间，
并且加上前面处理的结果。优先将最后一个值计算出来设置给promise。然后在利用for_each遍历计算其他位置的值。*/