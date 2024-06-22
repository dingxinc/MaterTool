#ifndef __ASYNC_LOG__
#define __ASYNC_LOG__

// �첽��־
// 2024-6-22

#include <thread>
#include <condition_variable>
#include <mutex>

#include <iostream>
#include <any>
#include <queue>
#include <sstream>

namespace AsyncLog {

    // ��־�ȼ�
    enum LogLv {
        DEBUGS = 0,
        INFO = 1,
        WARN = 2,
        ERRORS = 3,
    };

    // ҪͶ�ݸ����е�����ṹ
    class LogTask {
    public:
        LogTask() {}
        /* �������� */
        LogTask(const LogTask& src) :_level(src._level), _logdatas(src._logdatas) {}
        /* �ƶ����� */
        LogTask(const LogTask&& src) :_level(src._level),
            _logdatas(std::move(src._logdatas)) {}

    public:
        LogLv _level;                     // ��־�ȼ�
        std::queue<std::any> _logdatas;   // ��־����
    };

    class AsyncLog {
    public:
        /* �ֲ����� */
        static AsyncLog& Instance() {
            static AsyncLog instance;
            return instance;
        }

        ~AsyncLog() {
            Stop();
            workthread.join();
            std::cout << "exit success" << std::endl;
        }

        template<typename T>
        std::any toAny(const T& value) {
            return std::any(value);
        }

        //�����֧��C++11,�ɲ������º������
        template<typename Arg, typename ...Args>
        void TaskEnque(std::shared_ptr<LogTask> task, Arg&& arg, Args&&... args) {
            task->_logdatas.push(std::any(arg));
            TaskEnque(task, std::forward<Args>(args)...);
        }

        template<typename Arg>  // ����ͨ���ݹ���ã�����ʣ��һ�������İ汾
        void TaskEnque(std::shared_ptr<LogTask> task, Arg&& arg) {
            task->_logdatas.push(std::any(arg));
        }

        //�ɱ�����б��첽д
        template<typename...  Args>
        void AsyncWrite(LogLv level, Args&&... args) { // �������õĺô���֧����ֵ���ݡ�֧������ת��
            auto task = std::make_shared<LogTask>();
            //�۵����ʽ���ν��ɱ����д�����,��C++17�汾֧��
            (task->_logdatas.push(args), ...);
            //�粻֧��C++17 ��������汾���
            //TaskEnque(task, args...);
            task->_level = level;
            std::unique_lock<std::mutex> lock(_mtx);
            _queue.push(task);
            bool notify = (_queue.size() == 1) ? true : false;
            lock.unlock();
            // ֪ͨ�ȴ����߳����µ�����ɴ���
            if (notify) {
                _empty_cond.notify_one();
            }
        }

        void Stop() {
            _b_stop = true;
            _empty_cond.notify_one();
        }

    private:
        AsyncLog() :_b_stop(false) {
            workthread = std::thread([this]() {
                for (;;) {
                    std::unique_lock<std::mutex> lock(_mtx);
                    while (_queue.empty() && !_b_stop) {
                        _empty_cond.wait(lock);
                    }
                    if (_b_stop) {
                        return;
                    }
                    auto logtask = _queue.front();
                    _queue.pop();
                    lock.unlock();
                    processTask(logtask);
                }
            });
        }

        AsyncLog& operator =(const AsyncLog&) = delete;
        AsyncLog(const AsyncLog&) = delete;

        bool convert2Str(const std::any& data, std::string& str) {  // �� any ����ת��Ϊ�ַ�������
            std::ostringstream ss;
            if (data.type() == typeid(int)) {
                ss << std::any_cast<int>(data);
            }
            else if (data.type() == typeid(float)) {
                ss << std::any_cast<float>(data);
            }
            else if (data.type() == typeid(double)) {
                ss << std::any_cast<double>(data);
            }
            else if (data.type() == typeid(std::string)) {
                ss << std::any_cast<std::string>(data);
            }
            else if (data.type() == typeid(char*)) {
                ss << std::any_cast<char*>(data);
            }
            else if (data.type() == typeid(char const*)) {
                ss << std::any_cast<char const*>(data);
            }
            else {
                return false;
            }
            str = ss.str();
            return true;
        }

        void processTask(std::shared_ptr<LogTask> task) {
            std::cout << "log level is " << task->_level << std::endl;

            if (task->_logdatas.empty()) {
                return;
            }
            // ������Ԫ��
            auto head = task->_logdatas.front();
            task->_logdatas.pop();

            std::string formatstr = "";
            bool bsuccess = convert2Str(head, formatstr);
            if (!bsuccess) {
                return;
            }

            for (; !(task->_logdatas.empty());) {
                auto data = task->_logdatas.front();
                formatstr = formatString(formatstr, data);
                task->_logdatas.pop();
            }

            std::cout << "log string is " << formatstr << std::endl;
        }

        template<typename ...Args>
        std::string formatString(const std::string& format, Args... args) {
            std::string result = format;
            size_t pos = 0;
            //lambda���ʽ���Ҳ��滻�ַ���
            auto replacePlaceholder = [&](const std::string& placeholder, const std::any& replacement) {
                std::string str_replement = "";
                bool bsuccess = convert2Str(replacement, str_replement);
                if (!bsuccess) {
                    return;
                }

                size_t placeholderPos = result.find(placeholder, pos);
                if (placeholderPos != std::string::npos) {
                    result.replace(placeholderPos, placeholder.length(), str_replement);
                    pos = placeholderPos + str_replement.length();
                }
                else {
                    result = result + " " + str_replement;
                }
            };

            (replacePlaceholder("{}", args), ...);  // �۵����ʽ���ݹ��滻 {}
            return result;
        }

        std::condition_variable _empty_cond;
        std::queue<std::shared_ptr<LogTask> >  _queue;
        bool _b_stop;
        std::mutex _mtx;
        std::thread  workthread;
    };


    template<typename ... Args>
    void   ELog(Args&&... args) {
        AsyncLog::Instance().AsyncWrite(ERRORS, std::forward<Args>(args)...);
    }

    template<typename ... Args>
    void  DLog(Args&&... args) {
        AsyncLog::Instance().AsyncWrite(DEBUGS, std::forward<Args>(args)...);
    }

    template<typename ... Args>
    void  ILog(Args&&... args) {
        AsyncLog::Instance().AsyncWrite(INFO, std::forward<Args>(args)...);
    }

    template<typename ... Args>
    void  WLog(Args&&... args) {
        AsyncLog::Instance().AsyncWrite(WARN, std::forward<Args>(args)...);
    }
}

#endif