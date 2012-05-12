#ifndef APPLICATION_HXX
#define APPLICATION_HXX
#include <gtkmm/main.h>
#include "window.hxx"
#include "lurker.hxx"

namespace Derp {

  class Application {
  public:
    explicit Application(int argc, char *argv[]);
    void run();

  private:
	  const int LURKER_TIMEOUT_SECS;
	  Gtk::Main m_kit;
	  Derp::Window window_;
	  Derp::Lurker lurker_;
  };
}
#endif
