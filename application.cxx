#include "application.hxx"
#include <gtkmm.h>
#include <iostream>
#include <gdkmm/pixbufanimation.h>
#include <queue>
#include "config.h"

Derp::Application::Application(int argc, char*argv[]) : 
  num_downloading(0.0),
  num_downloaded(0.0),
  m_kit(argc, argv)
{
  try {
    auto refBuilder = Gtk::Builder::create_from_file(COLDWIND_GLADE_LOCATION);
    refBuilder->get_widget("mainWindow", m_window);
    refBuilder->get_widget("urlEntry", m_urlEntry);
    refBuilder->get_widget("goToggle", m_goButton);
    refBuilder->get_widget("progressBar", m_progressBar);
    refBuilder->get_widget("lurkSpinButton", m_lurkSpinButton);
    refBuilder->get_widget("fileChooserButton", m_fileChooserButton);
    refBuilder->get_widget("killMeImage", m_image);
    m_lurkAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("lurkAdjustment"));
    m_xAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("xAdjustment"));
    m_yAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("yAdjustment"));
    m_killmegif = Gdk::PixbufAnimation::create_from_file(COLDWIND_KILLMEGIF_LOCATION);
    m_fangpng = Gdk::Pixbuf::create_from_file(COLDWIND_FANGPNG_LOCATION);
    m_image->set(m_fangpng);

  } catch (const Glib::FileError& ex) {
    std::cerr << "FileError: " << ex.what() << std::endl;
    exit(1);
  } catch (const Gtk::BuilderError& ex) {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    exit(1);
  } catch (const Glib::MarkupError& ex) {
    std::cerr << "MarkupError: " << ex.what() << std::endl;
    exit(1);
  }

  m_goButton->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Application::signal_go));
  m_manager.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Application::download_finished));
  m_manager.signal_all_downloads_finished.connect(sigc::mem_fun(*this, &Derp::Application::downloads_finished));
  m_manager.signal_starting_downloads.connect(sigc::mem_fun(*this, &Derp::Application::starting_downloads));
  
  Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(m_lurker, &Derp::Lurker::run), true), 60);
}

void Derp::Application::run() {
  m_kit.run(*m_window);
}

void Derp::Application::signal_go() {
  if (m_goButton->get_active()) {
    m_goButton->set_active(false);

    // Start downloads and lurk.
    m_manager.download_async({ m_urlEntry->get_text(),
	  m_fileChooserButton->get_file(),
	  static_cast<int>(m_lurkAdjustment->get_value()),
	  static_cast<int>(m_xAdjustment->get_value()),
	  static_cast<int>(m_yAdjustment->get_value()) });
  }
}

void Derp::Application::starting_downloads(int num) {
  if (num > 0) {
    num_downloaded = 0;
    num_downloading = num;
    m_image->set(m_killmegif);
    update_progressBar();
  } else {
    std::cerr << "Unexpected call to starting downloads (0)?" << std::endl;
  }
}

void Derp::Application::download_finished() {
  num_downloaded++;
  update_progressBar();
}

void Derp::Application::downloads_finished(int, const Request& request) {
    m_image->set(m_fangpng);
    if (request.minutes > 0.0) {
      m_lurker.add_async(request);
    }
}

void Derp::Application::update_progressBar() {
  m_progressBar->set_fraction(static_cast<double>(num_downloaded) / static_cast<double>(num_downloading));
  m_progressBar->set_show_text(true);
  std::stringstream st;
  st << num_downloaded << " of " << num_downloading << " images downloaded";
  m_progressBar->set_text(st.str());
}
