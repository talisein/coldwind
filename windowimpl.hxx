#ifndef WINDOWIMPL_HXX
#define WINDOWIMPL_HXX

#include <sigc++/signal.h>
#include <memory>
//#include "request.hxx"
#include "manager.hxx"

namespace Derp {

    class Request;
    class ManagerResult;

	class WindowImpl {
	public:
		virtual ~WindowImpl();
		sigc::signal<bool, const Request&> signal_new_request;
		//virtual void starting_downloads(int num) = 0;
		//virtual void download_finished() = 0;
		//virtual void downloads_finished(int num, const Request& request) = 0;
		//virtual void download_error(const Derp::Error&) = 0;
		//virtual void update_progress(double) = 0;
		virtual void run() = 0;
        
        virtual void request_changed_state    (const std::shared_ptr<const ManagerResult>& result) = 0;
        virtual void request_download_complete(const std::shared_ptr<const ManagerResult>& result) = 0;
        virtual void request_error            (const std::shared_ptr<const ManagerResult>& result) = 0;
	protected:
		WindowImpl() = default;

	private:
		WindowImpl(const WindowImpl&) = delete;
		WindowImpl& operator=(const WindowImpl&) = delete;
	};
}

#endif
