#pragma once
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>

namespace Derp {

    template <typename T> class message_queue {
    public:
        message_queue<T>() = default;

        void push(T&& t) {
            std::lock_guard<std::mutex> lock(m);
            queue.push(std::forward<T>(t));
            cond.notify_one();
        };
            
        T pop() {
            std::unique_lock<std::mutex> lock(m);
            while (queue.empty()) {
                cond.wait(lock);
            }
            auto front = std::move(queue.front());
            queue.pop();
            return front;
        };

    private:
        std::queue<T> queue;
        std::mutex m;
        std::condition_variable cond;
    };

    class Active {
    public:
        typedef std::function<void()> Message;
        Active( const Active& ) = delete;
        void operator=( const Active& ) = delete;
 
    private:
        bool done;
        message_queue<Message> mq;
        std::unique_ptr<std::thread> thd;
 
        void Run() {
            while( !done ) {
                mq.pop()();
                //auto msg = mq.pop();
                //msg();            // execute message
            } // note: last message sets done to true
        }
 
    public:
         Active() : done(false) {
            thd = std::unique_ptr<std::thread>(
                new std::thread( [=]{ this->Run(); } ) );
        }
 
        ~Active() {
            send( [&]{ done = true; } ); ;
            thd->join();
        }
 
        template <typename Functor>
        void send( Functor&& m ) {
            mq.push( std::forward<Functor>(m) );
        }
    };

}
