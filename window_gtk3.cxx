#include "window_gtk3.hxx"
#include <iostream>
#include <gdkmm/pixbufanimation.h>

static Glib::RefPtr<Gdk::PixbufAnimation> get_animation_from_resource(const char* path) {
	auto loader = Gdk::PixbufLoader::create();
	GBytes* bytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	gsize bytes_size = 0;
	gconstpointer bytes_ptr = g_bytes_get_data(bytes, &bytes_size);
	loader->write(static_cast<const unsigned char*>(bytes_ptr), bytes_size);
	loader->close();
	g_bytes_unref(bytes);
	return loader->get_animation();
}

Derp::Window_Gtk3::Window_Gtk3(BaseObjectType* cobject,
                               const Glib::RefPtr<Gtk::Builder>& refBuilder) 
	: Derp::WindowImpl(), 
	  Gtk::Window(cobject),
	  num_downloading_(0),
	  num_downloaded_(0),
	  num_download_errors_(0),
	  progress_(0.0),
	  image_(nullptr)
{
    refBuilder->get_widget("urlEntry", urlEntry_);
    refBuilder->get_widget("threadFolderEntry", threadFolderEntry_);
    refBuilder->get_widget("goToggle", goButton_);
    refBuilder->get_widget("progressBar", progressBar_);
    refBuilder->get_widget("fileChooserButton", fileChooserButton_);
    refBuilder->get_widget("killMeImage", image_);
    refBuilder->get_widget("boardDirCheckbox", boardDirCheckbox_);
    refBuilder->get_widget("threadDirCheckbox", threadDirCheckbox_);
    refBuilder->get_widget("originalFilenameCheckbox", originalFilenameCheckbox_);
    refBuilder->get_widget("lurk404Checkbox", lurk404Checkbox_);
    refBuilder->get_widget("threadLabel", threadLabel_);
    refBuilder->get_widget("grid2", headerGrid_);

    lurkAdjustment_ = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("lurkAdjustment"));
    xAdjustment_ = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("xAdjustment"));
    yAdjustment_ = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("yAdjustment"));

    headerGrid_->set_focus_chain({urlEntry_, fileChooserButton_, threadFolderEntry_, goButton_});

    threadListStore_ = Gtk::ListStore::create(Derp::ThreadDirColumns::getInstance());
    entryCompletion_ = Gtk::EntryCompletion::create();
    entryCompletion_->set_model(threadListStore_);
    entryCompletion_->set_text_column(Derp::ThreadDirColumns::getFilenameColumn());
    entryCompletion_->set_inline_completion(true);
    entryCompletion_->set_inline_selection(false);
    entryCompletion_->set_popup_completion(true);
    threadFolderEntry_->set_completion(entryCompletion_);

    killmegif_ = get_animation_from_resource("/org/talinet/coldwind/KillMe.gif");
    errorgif_  = get_animation_from_resource("/org/talinet/coldwind/error.gif");
    fangpng_ = Glib::wrap(gdk_pixbuf_new_from_resource("/org/talinet/coldwind/fang.png", NULL));
    image_->set(fangpng_);

    urlEntry_->get_buffer()->signal_inserted_text().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_url_entry) );
    boardDirCheckbox_->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_board_toggled) );
    threadDirCheckbox_->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_thread_toggled) );
    fileChooserButton_->signal_file_set().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_board_toggled) );

    goButton_->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::goButton_click));

    on_thread_toggled(); // Show/Hide the entryCompletion based on xml
}

Derp::Window_Gtk3::~Window_Gtk3() {
	on_hide();
}

void Derp::Window_Gtk3::on_url_entry(guint, const gchar*, guint) {
  update_thread_dir_completer();
}

void Derp::Window_Gtk3::on_board_toggled() {
  if ( !boardDirCheckbox_->get_active() && !threadDirCheckbox_->get_active())
    threadLabel_->set_visible(false);
  else 
    threadLabel_->set_visible(true);

  update_thread_dir_completer();
}

void Derp::Window_Gtk3::on_thread_toggled() {
  if ( !boardDirCheckbox_->get_active() && !threadDirCheckbox_->get_active())
    threadLabel_->set_visible(false);
  else 
    threadLabel_->set_visible(true);

  if ( threadDirCheckbox_->get_active() ) {
    threadFolderEntry_->set_visible(true);
    update_thread_dir_completer();
  } else { 
    threadFolderEntry_->set_visible(false);
  }
}

void Derp::Window_Gtk3::update_thread_dir_finish(const Glib::RefPtr<Gio::AsyncResult>& result) {
	Glib::RefPtr<Gio::File> file = Glib::RefPtr<Gio::File>::cast_dynamic(result->get_source_object_base());
    
	auto enumerator = file->enumerate_children_finish(result);
	threadListStore_->clear();
	for ( auto info = enumerator->next_file(); info; info = enumerator->next_file() ) {
		auto enumerator = file->enumerate_children_finish(result);
		threadListStore_->clear();
		for ( auto info = enumerator->next_file(); info; info = enumerator->next_file() ) {
			if (info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY) {
				threadListStore_->append()->set_value(Derp::ThreadDirColumns::getFilenameColumn(), Glib::ustring(info->get_display_name()));
			}
		}
	}
	enumerator->close();
}

void Derp::Window_Gtk3::update_thread_dir_completer() {
  Glib::RefPtr<Gio::File> basedir = fileChooserButton_->get_file();
  Glib::RefPtr<Gio::File> dir;
  const Request req = { urlEntry_->get_text(), basedir, "", 0, 0, 0, true, true, true, false };
  threadFolderEntry_->set_text(req.getThread());
  
  // Delete all the current completions
  threadListStore_->clear();

  if (boardDirCheckbox_->get_active()) {
    threadLabel_->set_text(Glib::build_filename(" ",req.getBoard(), " "));
    dir = basedir->get_child_for_display_name(req.getBoard());
    if (! dir->query_exists() )
      return;
  } else {
    threadLabel_->set_text(Glib::build_filename(" ", " "));
    dir = basedir;
  }

  dir->enumerate_children_async(sigc::mem_fun(*this, &Derp::Window_Gtk3::update_thread_dir_finish),
				"standard::type,standard::display-name");
}

void Derp::Window_Gtk3::goButton_click() {
  if (goButton_->get_active()) {
    goButton_->set_active(false);

    num_download_errors_ = 0;
    bool is_accepted = signal_new_request({ urlEntry_->get_text(),
	  fileChooserButton_->get_file(),
	  threadFolderEntry_->get_text(),
	  static_cast<int>(lurkAdjustment_->get_value()),
	  static_cast<int>(xAdjustment_->get_value()),
	  static_cast<int>(yAdjustment_->get_value()),
	  boardDirCheckbox_->get_active(),
	  threadDirCheckbox_->get_active(),
	  originalFilenameCheckbox_->get_active(),
	  lurk404Checkbox_->get_active()
	  });
    if (is_accepted) {
      goButton_->set_sensitive(false);
    }
  }
}

void Derp::Window_Gtk3::run() {
	show_all();
}

void Derp::Window_Gtk3::on_hide() {
	Gtk::Window::on_hide();
}

void Derp::Window_Gtk3::starting_downloads(int num) {
  if (num > 0) {
    num_downloaded_ = 0;
    num_downloading_ = num;
    image_->set(killmegif_);
    progress_ = 0.0;
    update_progressBar();

  } else {
    std::cerr << "Unexpected call to starting downloads (0)?" << std::endl;
  }
}

void Derp::Window_Gtk3::download_error(const Derp::Error& error) {
  switch (error) {
  case THREAD_404:
    image_->set(errorgif_);
    progressBar_->set_text("Thread has 404ed");
    progressBar_->set_show_text(true);
    progressBar_->set_fraction(1.0);
    num_download_errors_++;
    goButton_->set_sensitive(true);
    break;
  default:
    num_download_errors_++;
    image_->set(errorgif_);
    update_progressBar();
  }
}

void Derp::Window_Gtk3::download_finished() {
  num_downloaded_++;
}

void Derp::Window_Gtk3::downloads_finished(int, const Request&) {
  if (num_download_errors_ == 0) {
    image_->set(fangpng_);
  }

  goButton_->set_sensitive(true);
  update_progressBar();
}

void Derp::Window_Gtk3::update_progress(double progress)
{
	progress_ = progress;
	update_progressBar();
}

void Derp::Window_Gtk3::update_progressBar() {
	progressBar_->set_fraction( progress_ );

	progressBar_->set_show_text(true);
	std::stringstream st;
	st << num_downloaded_ << " of " << num_downloading_ << " images downloaded";
	if (num_download_errors_ > 0) 
		st << ", " << num_download_errors_ << " error";
	if (num_download_errors_ > 1)
		st << "s";
	progressBar_->set_text(st.str());
}
