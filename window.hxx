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
		Window(const std::shared_ptr<Hasher>&);

		void run();
		sigc::signal<void, const Request&> signal_new_request;
		Derp::WindowImpl* getWindowImpl();

	protected:
		Derp::WindowImpl* createWindowImpl();

	private:
		const int PROGRESS_FPS;

		Derp::WindowImpl* uwindowImpl_;

		Glib::Timer timer_;
		Derp::Manager manager_;
		sigc::connection progressConnection_;

		bool startManager(const Request&);
		void onStartDownloads(int);
		void updateProgress();
		void onDownloadsFinished(int num, const Request& request);
		
	};
}

#endif
