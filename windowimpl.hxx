#ifndef WINDOWIMPL_HXX
#define WINDOWIMPL_HXX

#include <sigc++/signal.h>
#include "request.hxx"
#include "manager.hxx"

namespace Derp {
	
	class WindowImpl {
	public:
		virtual ~WindowImpl();
		sigc::signal<bool, const Request&> signal_new_request;
		virtual void starting_downloads(int num) = 0;
		virtual void download_finished() = 0;
		virtual void downloads_finished(int num, const Request& request) = 0;
		virtual void download_error(const Derp::Error&) = 0;

		virtual void update_progress(double) = 0;
		
	protected:
		WindowImpl() = default;

	private:
		WindowImpl(const WindowImpl&) = delete;
		WindowImpl& operator=(const WindowImpl&) = delete;
	};
}

#endif
