#pragma once
// ��������
// 2024-6-22

#include <iostream>
#include <list>
#include "../thread/thread_pool.h"

// ��������Quick Sort����һ�ָ�Ч�������㷨�����÷��η���˼��������������ǿ�������Ļ������裺

// 1. ѡ��һ����׼Ԫ�أ�pivot������������ѡ��һ��Ԫ����Ϊ��׼Ԫ�ء�ѡ���׼Ԫ�صķ�ʽ�кܶ��֣���������ѡ������ĵ�һ��Ԫ�ػ����һ��Ԫ�ء�
// 2. ������partitioning���������������飬�ѱȻ�׼Ԫ��С��Ԫ�ط���������ߣ��ѱȻ�׼Ԫ�ش��Ԫ�ط��������ұߡ���������̽���ʱ����׼Ԫ�ؾʹ������������λ�á�
// 3. �ݹ����������飺�ݹ�ضԻ�׼Ԫ����ߺ��ұߵ�������������п�������

/* �ݹ�������� */
template <typename T>
void quick_sort_recursive(T arr[], int start, int end)
{
    if (start >= end)
        return;
    T key = arr[start]; /* ��һ��Ԫ����Ϊ��׼Ԫ�� */
    int left = start, right = end;
    while (left < right)
    {
        while (arr[right] >= key && (left < right))
            right--;
        while (arr[left] <= key && (left < right))
            left++;
        std::swap(arr[left], arr[right]);
    }

    if (arr[left] < key)
    {
        std::swap(arr[left], arr[start]);
    }

    quick_sort_recursive(arr, start, left - 1); /* �ݹ�����ߵ� */
    quick_sort_recursive(arr, left + 1, end);   /* �ݹ����ұߵ� */
}

template <typename T>
void quick_sort(T arr[], int len)
{
    quick_sort_recursive(arr, 0, len - 1);
}

/* ��ͨ�õķ�ʽ�����ú���ʽ���˼�� */
template <typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
    if (input.empty()) /* input Ϊ���ŵ����� */
    {
        return input;
    }
    std::list<T> result; /* ������ɴ�Ž�������� */

    //  �� ��input�еĵ�һ��Ԫ�ط���result�У����ҽ����һ��Ԫ�ش�input��ɾ��
    result.splice(result.begin(), input, input.begin()); /* result �еĵ�һ��Ԫ�ؾ��ǻ�׼Ԫ�� */

    //  �� ȡresult�ĵ�һ��Ԫ�أ����������Ԫ�����и�и�input�е��б�
    T const& pivot = *result.begin();

    //  ��std::partition ��һ����׼�⺯�������ڽ������������е�Ԫ�ذ���ָ�����������з�����
    // ʹ������������Ԫ�����ڲ�����������Ԫ��֮ǰ��
    // ���Ծ�������divide_pointָ�����input�е�һ�����ڵ���pivot��Ԫ��
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t)
        { return t < pivot; });

    // �� ���ǽ�С��pivot��Ԫ�ط���lower_part��
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);

    // �����ǽ�lower_part���ݸ�sequential_quick_sort ����һ���µ�����Ĵ�С���������
    // lower_part �ж���С��divide_point��ֵ
    auto new_lower(
        sequential_quick_sort(std::move(lower_part)));
    // ������ʣ���input�б��ݸ�sequential_quick_sort�ݹ���ã�input�ж��Ǵ���divide_point��ֵ��
    auto new_higher(
        sequential_quick_sort(std::move(input)));
    // �ߵ���ʱnew_higher��new_lower���Ǵ�С��������õ��б�
    // ��new_higher ƴ�ӵ�result��β��
    result.splice(result.end(), new_higher);
    // ��new_lower ƴ�ӵ�result��ͷ��
    result.splice(result.begin(), new_lower);
    return result;
}

/* ���з�ʽ */
template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t)
        { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ����Ϊlower_part�Ǹ��������Բ��в������������߼����������������future������
    std::future<std::list<T>> new_lower(
        std::async(&parallel_quick_sort<T>, std::move(lower_part)));
    // async �൱�ڿ�����һ���̣߳���������̵߳��������ڲ���Ҫ����ȥ�ܣ����ǲ���ϵͳ�������ǲ��ܿ����̵߳Ŀ����͹ر�
    // ��һ����׼Ԫ���������֮����ߺ��ұ�������ʵ�������ţ����Բ��ò��еķ�ʽ��ͬʱ��������
    // ��
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

/* �̳߳ذ汾 */
template <typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input)
{
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t)
        { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(),
        divide_point);
    // ����Ϊlower_part�Ǹ��������Բ��в������������߼���������Ͷ�ݸ��̳߳ش���
    auto new_lower = ThreadPool::GetInstance()->commit(&parallel_quick_sort<T>, std::move(lower_part));
    // ��
    auto new_higher(
        parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}