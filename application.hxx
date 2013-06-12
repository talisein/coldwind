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
		const int LURKER_TIMEOUT_SECS;
		void on_my_startup();
        std::shared_ptr<Downloader> downloader_;
        std::shared_ptr<JsonParser> json_parser_;
        std::shared_ptr<Hasher> hasher_;
		Derp::Window window_;
		Derp::Lurker lurker_;
	};
}
#endif
