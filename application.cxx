#include "application.hxx"
#include <iostream>
#include <iomanip>
#include <queue>
#include "config.h"

Derp::Application::Application(int argc, char*argv[]) : 
  LURKER_TIMEOUT_SECS(60),
  num_downloading(0.0),
  num_downloaded(0.0),
  m_kit(argc, argv)
{
  try {
    auto refBuilder = Gtk::Builder::create_from_file(COLDWIND_GLADE_LOCATION);

    refBuilder->get_widget("mainWindow", m_window);
    refBuilder->get_widget("urlEntry", m_urlEntry);
    refBuilder->get_widget("threadFolderEntry", m_threadFolderEntry);
    refBuilder->get_widget("goToggle", m_goButton);
    refBuilder->get_widget("progressBar", m_progressBar);
    refBuilder->get_widget("lurkSpinButton", m_lurkSpinButton);
    refBuilder->get_widget("fileChooserButton", m_fileChooserButton);
    refBuilder->get_widget("killMeImage", m_image);
    refBuilder->get_widget("boardDirCheckbox", m_boardDirCheckbox);
    refBuilder->get_widget("threadDirCheckbox", m_threadDirCheckbox);
    refBuilder->get_widget("originalFilenameCheckbox", m_originalFilenameCheckbox);
    refBuilder->get_widget("lurk404Checkbox", m_lurk404Checkbox);
    refBuilder->get_widget("threadLabel", m_threadLabel);
    refBuilder->get_widget("grid2", m_headerGrid);

    m_lurkAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("lurkAdjustment"));
    m_xAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("xAdjustment"));
    m_yAdjustment = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("yAdjustment"));

    m_headerGrid->set_focus_chain({m_urlEntry, m_fileChooserButton, m_threadFolderEntry, m_goButton});

    m_threadListStore = Gtk::ListStore::create(Derp::ThreadDirColumns::getInstance());
    m_entryCompletion = Gtk::EntryCompletion::create();
    m_entryCompletion->set_model(m_threadListStore);
    m_entryCompletion->set_text_column(Derp::ThreadDirColumns::getFilenameColumn());
    m_entryCompletion->set_inline_completion(true);
    m_entryCompletion->set_inline_selection(false);
    m_entryCompletion->set_popup_completion(true);
    m_threadFolderEntry->set_completion(m_entryCompletion);
    on_thread_toggled(); // Show/Hide the entryCompletion based on xml

    m_killmegif = Gdk::PixbufAnimation::create_from_file(COLDWIND_KILLMEGIF_LOCATION);
    m_fangpng = Gdk::Pixbuf::create_from_file(COLDWIND_FANGPNG_LOCATION);
    m_errorgif = Gdk::PixbufAnimation::create_from_file(COLDWIND_ERRORGIF_LOCATION);
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
  
  m_image->set(m_fangpng);

  m_goButton->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Application::signal_go));
  m_urlEntry->get_buffer()->signal_inserted_text().connect( sigc::mem_fun(*this, &Derp::Application::on_url_entry) );
  m_boardDirCheckbox->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Application::on_board_toggled) );
  m_threadDirCheckbox->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Application::on_thread_toggled) );
  m_fileChooserButton->signal_file_set().connect( sigc::mem_fun(*this, &Derp::Application::on_board_toggled) );
  m_manager.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Application::download_finished));
  m_manager.signal_download_error.connect(sigc::mem_fun(*this, &Derp::Application::download_error));
  m_manager.signal_all_downloads_finished.connect(sigc::mem_fun(*this, &Derp::Application::downloads_finished));
  m_manager.signal_starting_downloads.connect(sigc::mem_fun(*this, &Derp::Application::starting_downloads));
  
  Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(m_lurker, &Derp::Lurker::run), true), LURKER_TIMEOUT_SECS);
}

void Derp::Application::run() {
  m_kit.run(*m_window);
}

void Derp::Application::on_board_toggled() {
  if ( !m_boardDirCheckbox->get_active() && !m_threadDirCheckbox->get_active())
    m_threadLabel->set_visible(false);
  else 
    m_threadLabel->set_visible(true);

  update_thread_dir_completer();
}

void Derp::Application::on_thread_toggled() {
  if ( !m_boardDirCheckbox->get_active() && !m_threadDirCheckbox->get_active())
    m_threadLabel->set_visible(false);
  else 
    m_threadLabel->set_visible(true);

  if ( m_threadDirCheckbox->get_active() ) {
    m_threadFolderEntry->set_visible(true);
    update_thread_dir_completer();
  } else { 
    m_threadFolderEntry->set_visible(false);
  }
}

void Derp::Application::on_url_entry(guint, const gchar*, guint) {
  update_thread_dir_completer();
}

void Derp::Application::update_thread_dir_finish(const Glib::RefPtr<Gio::AsyncResult>& result) {
  Glib::RefPtr<Gio::File> file = Glib::RefPtr<Gio::File>::cast_dynamic(result->get_source_object_base());
    
  auto enumerator = file->enumerate_children_finish(result);
  m_threadListStore->clear();
  for ( auto info = enumerator->next_file(); info; info = enumerator->next_file() ) {
    if (info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY) {
      m_threadListStore->append()->set_value(Derp::ThreadDirColumns::getFilenameColumn(), Glib::ustring(info->get_display_name()));
    }
  }
  enumerator->close();
}

void Derp::Application::update_thread_dir_completer() {
  Glib::RefPtr<Gio::File> basedir = m_fileChooserButton->get_file();
  Glib::RefPtr<Gio::File> dir;
  const Request req = { m_urlEntry->get_text(), basedir, "", 0, 0, 0, true, true, true, false };
  m_threadFolderEntry->set_text(req.getThread());
  
  // Delete all the current completions
  m_threadListStore->clear();

  if (m_boardDirCheckbox->get_active()) {
    m_threadLabel->set_text(Glib::build_filename(" ",req.getBoard(), " "));
    dir = basedir->get_child_for_display_name(req.getBoard());
    if (! dir->query_exists() )
      return;
  } else {
    m_threadLabel->set_text(Glib::build_filename(" ", " "));
    dir = basedir;
  }

  dir->enumerate_children_async(sigc::mem_fun(*this, &Derp::Application::update_thread_dir_finish),
				"standard::type,standard::display-name");
}

void Derp::Application::signal_go() {
  if (m_goButton->get_active()) {
    m_goButton->set_active(false);
    
    // Start downloads and lurk.
    m_timer.reset();
    m_timer.start();
    num_download_errors = 0;
    bool is_accepted = m_manager.download_async({ m_urlEntry->get_text(),
	  m_fileChooserButton->get_file(),
	  m_threadFolderEntry->get_text(),
	  static_cast<int>(m_lurkAdjustment->get_value()),
	  static_cast<int>(m_xAdjustment->get_value()),
	  static_cast<int>(m_yAdjustment->get_value()),
	  m_boardDirCheckbox->get_active(),
	  m_threadDirCheckbox->get_active(),
	  m_originalFilenameCheckbox->get_active(),
	  m_lurk404Checkbox->get_active()
	  });
    if (is_accepted) {
      m_goButton->set_sensitive(false);
    }
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

void Derp::Application::download_error(const Derp::Error& error) {
  switch (error) {
  case THREAD_404:
    m_image->set(m_errorgif);
    m_progressBar->set_text("Thread has 404ed");
    m_progressBar->set_show_text(true);
    m_progressBar->set_fraction(1.0);
    num_download_errors++;
    m_timer.stop();
    m_goButton->set_sensitive(true);
    break;
  default:
    num_download_errors++;
    m_image->set(m_errorgif);
    update_progressBar();
  }
}

void Derp::Application::download_finished() {
  num_downloaded++;
  update_progressBar();
}

void Derp::Application::downloads_finished(int, const Request& request) {
  m_timer.stop();
  std::cout << "Downloaded images in " << std::setprecision(5) << m_timer.elapsed() << " seconds." << std::endl;
  if (num_download_errors == 0) {
    m_image->set(m_fangpng);
  }

  m_goButton->set_sensitive(true);
  if (!request.isExpired()) {
    m_lurker.add_async(request);
  }
  update_progressBar();
}

void Derp::Application::update_progressBar() {
  m_progressBar->set_fraction(static_cast<double>(num_downloaded + num_download_errors) / static_cast<double>(num_downloading));
  m_progressBar->set_show_text(true);
  std::stringstream st;
  st << num_downloaded << " of " << num_downloading << " images downloaded";
  if (num_download_errors > 0) 
    st << ", " << num_download_errors << " error";
  if (num_download_errors > 1)
    st << "s";
  m_progressBar->set_text(st.str());
}
