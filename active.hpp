#pragma once
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <deque>

namespace Derp {

    template <typename T> class message_queue {
    public:
        message_queue<T>() = default;

        template <typename U>
        void push(U&& u) {
            std::lock_guard<std::mutex> lock(m);
            queue.push_back(std::forward<U>(u));
            cond.notify_one();
        }

        template <typename U>
        void push_front(U&& u) {
            std::lock_guard<std::mutex> lock(m);
            queue.push_front(std::forward<U>(u));
            cond.notify_one();
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            std::lock_guard<std::mutex> lock(m);
            queue.emplace_back(std::forward<Args>(args)...);
            cond.notify_one();
        }
            
        T pop() {
            std::unique_lock<std::mutex> lock(m);
            while (queue.empty()) {
                cond.wait(lock);
            }
            auto front = std::move(queue.front());
            queue.pop_front();
            return front;
        };

    private:
        std::deque<T> queue;
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
            } // note: last message sets done to true
        }
 
    public:
         Active() : done(false) {
            thd = std::unique_ptr<std::thread>(
                new std::thread( std::mem_fn(&Active::Run), this ) );
        }
 
        ~Active() {
            send_priority( [&]{ done = true; } ); ;
            thd->join();
        }
 
        template <typename Functor>
        void send( Functor&& m ) {
            mq.push( std::forward<Functor>(m) );
        }

        template <typename Functor>
        void send_priority( Functor&& m) {
            mq.push_front( std::forward<Functor>(m) );
        }

        template <typename... Args>
        void emplace(Args&&... args) {
            mq.emplace(std::forward<Args>(args)...);
        }
    };
}
