#include "application.hxx"
#include "hasher.hxx"
#include <iostream>

Derp::Application::Application(int argc, char* argv[]) : 
	Gtk::Application(argc, 
	                 argv,
	                 "org.talinet.coldwind",
	                 Gio::APPLICATION_FLAGS_NONE),
	window_(std::make_shared<Manager>())
{
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
