#include "window.hxx"
#include <iostream>
#include <iomanip>
#include "config.h"
#include "window_gtk3.hxx"

Derp::Window::Window(const std::shared_ptr<Hasher>& hasher) 
	: PROGRESS_FPS(60),
	  uwindowImpl_(createWindowImpl()),
	  timer_(),
	  manager_(hasher),
	  progressConnection_()
{
	// Signals WindowImpl catches
	manager_.signal_starting_downloads.connect(sigc::mem_fun(uwindowImpl_, &Derp::WindowImpl::starting_downloads));
	manager_.signal_download_finished.connect(sigc::mem_fun(uwindowImpl_, &Derp::WindowImpl::download_finished));
	manager_.signal_all_downloads_finished.connect(sigc::mem_fun(uwindowImpl_, &Derp::WindowImpl::downloads_finished));
	manager_.signal_download_error.connect(sigc::mem_fun(uwindowImpl_, &Derp::WindowImpl::download_error));
	

	// Signals Window catches
	manager_.signal_starting_downloads.connect( sigc::mem_fun(*this, &Derp::Window::onStartDownloads) );
	manager_.signal_all_downloads_finished.connect(sigc::mem_fun(*this, &Derp::Window::onDownloadsFinished));
}

Derp::WindowImpl* Derp::Window::getWindowImpl() {
	return uwindowImpl_;
}

Derp::WindowImpl* Derp::Window::createWindowImpl() {
	// For now, just create GTK3 since that's the only impl
	Window_Gtk3* impl = nullptr;

	try {

		GtkBuilder* cbuilder = gtk_builder_new();
		gtk_builder_add_from_resource(cbuilder, "/org/talinet/coldwind/overEngineering.glade", NULL);
		Glib::RefPtr<Gtk::Builder> refBuilder = Glib::wrap(cbuilder);
		refBuilder->get_widget_derived("mainWindow", impl);
		impl->signal_new_request.connect( sigc::mem_fun(*this, &Derp::Window::startManager) );

	} catch (const Glib::FileError& ex) {
		std::cerr << "FileError: " << ex.what() << std::endl;
		exit(EXIT_FAILURE);
	} catch (const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		exit(EXIT_FAILURE);
	} catch (const Glib::MarkupError& ex) {
		std::cerr << "MarkupError: " << ex.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	
	return impl;
}

void Derp::Window::run() {
	uwindowImpl_->run();
}

bool Derp::Window::startManager(const Request& request) {
    timer_.reset();
    timer_.start();

    bool is_accepted = manager_.download_async(request);
    if (!is_accepted) {
	    timer_.stop();
    }

    return is_accepted;
}

void Derp::Window::onStartDownloads(int) {
}

void Derp::Window::updateProgress() {
}

void Derp::Window::onDownloadsFinished(int num, const Derp::Request& request) {
	timer_.stop();
	std::cout << "Info: Downloaded " << num << " images in " << std::setprecision(5) << timer_.elapsed() << " seconds." << std::endl;

	progressConnection_.disconnect();

	if (!request.isExpired()) {
		signal_new_request(request);
	}
}

