#include "application.hxx"
#include <gtkmm.h>
#include <iostream>
#include <gdkmm/pixbufanimation.h>
#include <queue>
#include "config.h"

Derp::Application::Application(int argc, char*argv[]) : 
  num_downloading(0.0),
  num_downloaded(0.0),
  m_kit(argc, argv),
  isGif(false)
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
  m_parser.signal_parsing_finished.connect(sigc::mem_fun(*this, &Derp::Application::parsing_finished));
  m_hasher.signal_hashing_finished.connect(sigc::mem_fun(*this, &Derp::Application::hashing_finished));
  m_downloader.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Application::download_finished));
}

void Derp::Application::run() {
  m_kit.run(*m_window);
}

void Derp::Application::signal_go() {
  if (m_goButton->get_active()) {
    // Start downloads and lurk.
    m_goButton->set_sensitive(false);
    
    is_hashing = is_parsing = true;
    m_parser.parse_async(m_urlEntry->get_text());
    m_hasher.hash_async(m_fileChooserButton->get_file());
    m_goButton->set_active(false);
  } else {

  }
}

void Derp::Application::parsing_finished() {
  is_parsing = false;
  try_download();
}

void Derp::Application::hashing_finished() {
  is_hashing = false;
  try_download();
}

void Derp::Application::try_download() {
  if ( !(is_parsing || is_hashing) ) {
    num_downloaded = 0;
    int xDim = static_cast<int>(m_xAdjustment->get_value());
    int yDim = static_cast<int>(m_yAdjustment->get_value());
    int count = m_parser.request_downloads(m_downloader, &m_hasher, m_fileChooserButton->get_file()->get_path(), xDim, yDim);
    if (count > 0) {
      num_downloading = count;
      update_progressBar();
    } else {
      m_goButton->set_sensitive(true);
    }
  }
}

void Derp::Application::download_finished() {
  num_downloaded++;
  update_progressBar();
}

void Derp::Application::update_progressBar() {
  if (num_downloading != num_downloaded) {
    if (!isGif) {
      isGif = true;
      m_image->set(m_killmegif);
    }
  } else {
    m_image->set(m_fangpng);
    isGif = false;
    m_goButton->set_sensitive(true);
  }
  m_progressBar->set_fraction(static_cast<double>(num_downloaded) / static_cast<double>(num_downloading));
  m_progressBar->set_show_text(true);
  std::stringstream st;
  st << num_downloaded << " of " << num_downloading << " images downloaded";
  m_progressBar->set_text(st.str());
}
