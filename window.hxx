#ifndef WINDOW_HXX
#define WINDOW_HXX

#include <memory>
#include <sigc++/connection.h>
#include <glibmm/timer.h>
#include <gtkmm/window.h>
#include "windowimpl.hxx"
#include "manager.hxx"

namespace Derp {

	class Window {
	public:
		Window(const std::shared_ptr<Manager>&);

		void run();
		sigc::signal<void, const Request&> signal_new_request;
		WindowImpl* getWindowImpl();

	protected:
		WindowImpl* createWindowImpl();

	private:
        std::unique_ptr<WindowImpl> uwindowImpl_;

        std::shared_ptr<Manager> manager_;

		bool startManager(const Request&);

		void onDownloadsFinished(int num, const Request& request);

        void manager_cb(const std::shared_ptr<const ManagerResult>& result);
	};
}

#endif
