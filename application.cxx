#include "application.hxx"

Derp::Application::Application(int argc, char* argv[]) : 
	LURKER_TIMEOUT_SECS(60),
	m_kit(argc, argv),
	window_(),
	lurker_()
{
	Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(lurker_, &Derp::Lurker::run), true), LURKER_TIMEOUT_SECS);

	window_.signal_new_request.connect(sigc::mem_fun(lurker_,
	                                                 &Derp::Lurker::add_async));
}

void Derp::Application::run() {
	m_kit.run();
}
