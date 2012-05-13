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
		Window();

		void run();
		sigc::signal<void, const Request&> signal_new_request;

	protected:
		std::unique_ptr<Derp::WindowImpl> getWindowImpl();

	private:
		const int PROGRESS_FPS;

		Derp::WindowImpl* windowImpl_;
		std::unique_ptr<Derp::WindowImpl> uwindowImpl_;

		Glib::Timer timer_;
		Derp::Manager manager_;
		sigc::connection progressConnection_;

		bool hide_window(GdkEventAny*);

		bool startManager(const Request&);
		void onStartDownloads(int);
		void updateProgress();
		void onDownloadsFinished(int num, const Request&);
		
	};
}

#endif
