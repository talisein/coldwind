#ifndef APPLICATION_HXX
#define APPLICATION_HXX
#include <gtkmm/application.h>
#include "window.hxx"
#include "lurker.hxx"

namespace Derp {
    class JsonParser;
    class Downloader;

	class Application : public Gtk::Application {
	public:
		explicit Application(int argc, char *argv[]);
		void run();

	private:
		void on_my_startup();

		Derp::Window window_;
	};
}
#endif
