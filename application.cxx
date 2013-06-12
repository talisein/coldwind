#include "application.hxx"
#include "hasher.hxx"
#include <iostream>
Derp::Application::Application(int argc, char* argv[]) : 
	Gtk::Application(argc, 
	                 argv,
	                 "org.talinet.coldwind",
	                 Gio::APPLICATION_FLAGS_NONE),
	LURKER_TIMEOUT_SECS(60),
    hasher_(std::make_shared<Hasher>()),
	window_(hasher_),
	lurker_(hasher_)
{
    hasher_.reset();
	Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(lurker_, &Derp::Lurker::run), true), LURKER_TIMEOUT_SECS);

	window_.signal_new_request.connect(sigc::mem_fun(lurker_, 
	                                                 &Derp::Lurker::add_async));
	signal_startup().connect(sigc::mem_fun(*this, &Derp::Application::on_my_startup));
}

void Derp::Application::run() {
	if (!register_application()) {
		std::cerr << "Error registering application on D-Bus." << std::endl;
	}
}

void Derp::Application::on_my_startup() {
	Gtk::Application::run( dynamic_cast<Gtk::Window&>(*(window_.getWindowImpl())) );
}
