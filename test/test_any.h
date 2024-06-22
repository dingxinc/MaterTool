#pragma once
#include "../tool/Any.h"
using namespace mater;
#include <string>
#include <vector>

struct Student {
    Student() = default;
    Student(int id, std::string name) : id_(id), name_(std::move(name)) {}
    int id_{};
    std::string name_;
};

void test_any() {
    any t = "你好";
    auto test_construct = [&]() {
        t = 345456;

        t = Student();

        t = any(Student());

        t = std::move(any("hello我k$"));
    };

    auto test_cast = [&]() {
        t = std::to_string(43324324);
    };

    auto test_other = [&]() {
        t.emplace<Student>(323, std::string("fdfsafsda"));

        t.reset();

        any p = "再次测试看看";
        swap(t, p);

        t = std::vector<std::string>{ "fdasdgdg", "fdgdasedg", "dagdfegs" };

        t = "你好";
    };
    test_construct();
    test_cast();
    test_other();
}