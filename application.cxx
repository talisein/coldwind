#include "application.hxx"
#include <gtkmm.h>
#include <iostream>
#include <gdkmm/pixbufanimation.h>
#include <queue>

Derp::Application::Application(int argc, char*argv[]) : 
  num_downloading(0.0),
  num_downloaded(0.0),
  m_kit(argc, argv),
  isGif(false)
{
  try {
    auto refBuilder = Gtk::Builder::create_from_file("overEngineering.glade");
    refBuilder->get_widget("mainWindow", m_window);
    refBuilder->get_widget("urlEntry", m_urlEntry);
    refBuilder->get_widget("goToggle", m_goButton);
    refBuilder->get_widget("progressBar", m_progressBar);
    refBuilder->get_widget("lurkSpinButton", m_lurkSpinButton);
    refBuilder->get_widget("fileChooserButton", m_fileChooserButton);
    refBuilder->get_widget("killMeImage", m_image);
    m_killmegif = Gdk::PixbufAnimation::create_from_file("KillMe.gif");
    m_fangpng = Gdk::Pixbuf::create_from_file("fang.png");
    m_image->set(m_fangpng);
    
    m_goButton->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Application::signal_go));
    is_parsing = is_hashing = true;
    m_parser.signal_parsing_finished.connect(sigc::mem_fun(*this, &Derp::Application::parsing_finished));
    m_hasher.signal_hashing_finished.connect(sigc::mem_fun(*this, &Derp::Application::hashing_finished));
    m_downloader.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Application::download_finished));

  } catch (const Glib::FileError& ex) {
    std::cerr << "FileError: " << ex.what() << std::endl;
    
  } catch (const Gtk::BuilderError& ex) {
    std::cerr << "BuilderError: " << ex.what() << std::endl;
    
  } catch (const Glib::MarkupError& ex) {
    std::cerr << "MarkupError: " << ex.what() << std::endl;
  }

  /*
  // Connect file production signal
  Gtk::ImageMenuItem* openFileItem = 0;
  refBuilder->get_widget("openFileMenuItem", openFileItem);
  if (openFileItem) {
    openFileItem->signal_activate().connect( sigc::mem_fun(this, &CAPViewer::Application::signal_open_from_file ));
  } else {
    // TODO: Error condition
  }
  */
}

void Derp::Application::run() {
  m_kit.run(*m_window);
}

void Derp::Application::signal_go() {
  if (m_goButton->get_active()) {
    // Start downloads and lurk.
    m_lurkSpinButton->set_sensitive(false);
    double adj = m_lurkSpinButton->get_adjustment()->get_value();
    
    num_downloading = 0;
    num_downloaded = 0;
    m_progressBar->set_fraction(0.0);
    is_hashing = is_parsing = true;
    m_parser.parse_async(m_urlEntry->get_text());
    m_hasher.hash_async(m_fileChooserButton->get_file());
    if (adj > 0.0) {
      m_goButton->set_label("Lurking...");
      // Lurk.
    } else {
      m_goButton->set_active(false);
    }
  } else {
    // Cancel lurking.
    m_lurkSpinButton->set_sensitive(true);
    m_goButton->set_label("Go");
  }
}

void Derp::Application::parsing_finished() {
  std::cout << "Parsing of 4chan thread complete!" << std::endl;
  is_parsing = false;
  try_download();
}

void Derp::Application::hashing_finished() {
  std::cout << "Hashing of target directory complete!" << std::endl;
  is_hashing = false;
  try_download();
}

void Derp::Application::try_download() {
  if ( !is_parsing && !is_hashing ) {
    auto map = m_parser.get_image_sources();
    std::queue<Glib::ustring> sources;
    for( auto it = map.begin(); it != map.end(); it++ ) {
      if ( !m_hasher.is_downloaded(it->second.find("md5hex")->second) ) {
	// todo: Other filtering here
	sources.push(it->first);
      }
    }

    if (!sources.empty()) {
      num_downloading = sources.size();
      num_downloaded = 0;
      update_progressBar();
      m_downloader.download_async(sources, m_fileChooserButton->get_file()->get_path());
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
  }
  m_progressBar->set_fraction(static_cast<double>(num_downloaded) / static_cast<double>(num_downloading));
  m_progressBar->set_show_text(true);
  std::stringstream st;
  st << num_downloaded << " of " << num_downloading << " images downloaded";
  m_progressBar->set_text(st.str());
}
