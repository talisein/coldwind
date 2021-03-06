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

namespace {
    const std::string resource_str {"/org/talinet/coldwind/coldwind.glade"};
}

Derp::WindowImpl* Derp::Window::createWindowImpl() {
	// For now, just create GTK3 since that's the only impl
	Window_Gtk3* impl = nullptr;
	try {
		auto builder = Gtk::Builder::create_from_resource(resource_str);
		builder->get_widget_derived("coldwind_main_window", impl);
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

	return dynamic_cast<Derp::WindowImpl*>(impl);
}

void Derp::Window::run() {
	uwindowImpl_->run();
}

bool Derp::Window::startManager(const Request& request) {
    auto cb = [this](const std::shared_ptr<const ManagerResult> &result) {
        manager_cb(result);
    };
    bool is_accepted = manager_->download_async(request, cb);

    return is_accepted;
}

namespace Derp {
    void
    Window::manager_cb(const std::shared_ptr<const ManagerResult>& result)
    {
        switch (result->state) {
            case ManagerResult::HASHING:
            case ManagerResult::PARSING:
                if (result->had_error) {
                    uwindowImpl_->request_error(result);
                } else {
                    uwindowImpl_->request_changed_state(result);
                }
                break;
            case ManagerResult::LURKING:
                uwindowImpl_->request_changed_state(result);
                break;
            case ManagerResult::DOWNLOADING:
                    if (result->num_downloaded == 0 && result->num_download_errors == 0 ) {
                        uwindowImpl_->request_changed_state(result);
                    } else {
                        uwindowImpl_->request_download_complete(result);
                    }
                break;
            case ManagerResult::DONE:
                if (result->had_error) {
                    uwindowImpl_->request_error(result);
                } else {
                    uwindowImpl_->request_changed_state(result);
                }
                break;
            case ManagerResult::ERROR:
                uwindowImpl_->request_error(result);
                break;
        }
    }
}
