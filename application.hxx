#ifndef APPLICATION_HXX
#define APPLICATION_HXX
#include <gtkmm/application.h>
#include "window.hxx"
#include "lurker.hxx"

namespace Derp {

	class Application : public Gtk::Application {
	public:
		explicit Application(int argc, char *argv[]);
		void run();

	private:
		const int LURKER_TIMEOUT_SECS;
		void on_my_startup();
		Derp::Window window_;
		Derp::Lurker lurker_;
	};
}
#endif
