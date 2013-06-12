#include "window.hxx"
#include <iostream>
#include <iomanip>
#include "config.h"
#include "window_gtk3.hxx"

Derp::Window::Window(const std::shared_ptr<Manager>& manager) :
	  uwindowImpl_(createWindowImpl()),
	  manager_(manager)
{
}

Derp::WindowImpl* Derp::Window::getWindowImpl() {
	return uwindowImpl_.get();
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

    bool is_accepted = manager_->download_async(request,
                                               std::bind(&Derp::Window::manager_cb,
                                                         this,
                                                         std::placeholders::_1));

    return is_accepted;
}

void Derp::Window::onDownloadsFinished(int, const Derp::Request& request) {
	if (!request.isExpired()) {
		signal_new_request(request);
	}
}

namespace Derp {
    void
    Window::manager_cb(const std::shared_ptr<const ManagerResult>& result)
    {
        switch (result->state) {
            case ManagerResult::HASHING:
            case ManagerResult::PARSING:
            case ManagerResult::LURKING:
                break;
            case ManagerResult::DOWNLOADING:
                if (result->had_error) {
                    uwindowImpl_->download_error(result->error_code);
                } else if (result->num_downloaded > 0) {
                    uwindowImpl_->download_finished();
                } else {
                    uwindowImpl_->starting_downloads(result->num_downloading);
                }
                break;
            case ManagerResult::DONE:
                if (result->had_error) {
                    uwindowImpl_->download_error(result->error_code);
                } else {
                    uwindowImpl_->downloads_finished(result->num_downloaded, result->request);
                    onDownloadsFinished(result->num_downloaded, result->request);
                }
                break;
            case ManagerResult::ERROR:
                uwindowImpl_->download_error(result->error_code);
                break;
        }
    }
}
