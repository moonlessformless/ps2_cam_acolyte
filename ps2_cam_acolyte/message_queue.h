#pragma once

#include <thread>
#include "readerwriterqueue.h"

class message_queue
{
public:
    using func = std::function<void()>;
private:
    moodycamel::ReaderWriterQueue<func> queue;
    std::thread::id receive_thread;

public:
    message_queue()
        : receive_thread(std::this_thread::get_id())
    {
    }

    template<typename F>
    void send(F&& f)
    {
        queue.try_enqueue(std::move(f));
    }

    void receive()
    {
        assert(std::this_thread::get_id() == receive_thread);
        while (true)
        {
            func* f = queue.peek();
            if (f != nullptr)
            {
                (*f)();
                queue.pop();
            }
            else
            {
                break;
            }
        }
    }
};