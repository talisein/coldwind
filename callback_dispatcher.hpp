#pragma once
#include <mutex>
#include <queue>
#include <functional>
#include <glibmm/dispatcher.h>

namespace Derp {
    class CallbackDispatcher {
    public:
        CallbackDispatcher(const CallbackDispatcher&) = delete;
        CallbackDispatcher& operator=(const CallbackDispatcher&) = delete;

        CallbackDispatcher() {
            m_dispatcher.connect(sigc::mem_fun(*this,
                                               &CallbackDispatcher::on_dispatch));
        };

        template<typename Fn>
        void operator()(Fn&& fn) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_msgs.push(std::forward<Fn>(fn));
            m_dispatcher();
        }

    private:
        typedef std::function<void ()> Callback;

        std::queue<Callback> m_msgs;
        mutable std::mutex   m_mutex;
        Glib::Dispatcher     m_dispatcher;

        void on_dispatch() {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_msgs.empty()) {
                m_msgs.front()();
                m_msgs.pop();
            }
        }
    };
}
