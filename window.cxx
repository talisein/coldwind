#include "window.hxx"
#include <iostream>
#include <iomanip>
#include "config.h"
#include "window_gtk3.hxx"

Derp::Window::Window() 
	: PROGRESS_FPS(60),
	  windowImpl_(getWindowImpl()),
	  timer_(),
	  manager_(),
	  progressConnection_()
{
	// Signals WindowImpl catches
	manager_.signal_starting_downloads.connect(sigc::mem_fun(windowImpl_, &Derp::WindowImpl::starting_downloads));
	manager_.signal_download_finished.connect(sigc::mem_fun(windowImpl_, &Derp::WindowImpl::download_finished));
	manager_.signal_all_downloads_finished.connect(sigc::mem_fun(windowImpl_, &Derp::WindowImpl::downloads_finished));
	manager_.signal_download_error.connect(sigc::mem_fun(windowImpl_, &Derp::WindowImpl::download_error));
  

	// Signals Window catches
	windowImpl_->signal_new_request.connect( sigc::mem_fun(*this, &Derp::Window::startManager) );
	manager_.signal_starting_downloads.connect( sigc::mem_fun(*this, &Derp::Window::onStartDownloads) );
	manager_.signal_all_downloads_finished.connect(sigc::mem_fun(*this, &Derp::Window::onDownloadsFinished));
	
}

Derp::WindowImpl* Derp::Window::getWindowImpl() {
	// For now, just create GTK3 since that's the only impl
	Window_Gtk3* impl = nullptr;

	try {
		auto refBuilder = Gtk::Builder::create_from_file(COLDWIND_GLADE_LOCATION);
		refBuilder->get_widget_derived("mainWindow", impl);

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
	
	return dynamic_cast<Derp::WindowImpl*>(impl);
}

Gtk::Window& Derp::Window::run() {
	return dynamic_cast<Gtk::Window&>(*windowImpl_);
}

bool Derp::Window::startManager(const Request& request) {
    timer_.reset();
    timer_.start();

    bool is_accepted = manager_.download_async(request);
    if (!is_accepted) {
	    timer_.stop();
    }
    std::cerr << "Window: is_accepted is " << is_accepted << std::endl;

    return is_accepted;
}

void Derp::Window::onStartDownloads(int num) {
	if (num > 0)
		progressConnection_ = Glib::signal_timeout().connect( sigc::bind_return( sigc::mem_fun(*this, &Derp::Window::updateProgress), true), 1000 / PROGRESS_FPS);
}

void Derp::Window::updateProgress() {
	windowImpl_->update_progress( manager_.getProgress() );
}

void Derp::Window::onDownloadsFinished(int num, const Request& request) {
	timer_.stop();
	std::cout << "Info: Downloaded " << num << " images in " << std::setprecision(5) << timer_.elapsed() << " seconds." << std::endl;

	progressConnection_.disconnect();
	
	if (!request.isExpired()) {
		signal_new_request(request);
	}
}

