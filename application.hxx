// Derp
#ifndef APPLICATION_HXX
#define APPLICATION_HXX
#include <gtkmm.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/refptr.h>
#include <glibmm/timer.h>
#include <gdkmm/pixbufanimation.h>
#include <gtkmm/main.h>
#include <gtkmm/image.h>
#include <gtkmm/entrycompletion.h>
#include "parser.hxx"
#include "hasher.hxx"
#include "downloader.hxx"
#include "lurker.hxx"

namespace Derp {

 class ThreadDirColumns : public Gtk::TreeModel::ColumnRecord
 {
 public:
   Gtk::TreeModelColumn<Glib::ustring> filename;

   static ThreadDirColumns& getInstance() { static ThreadDirColumns S; return S; }
   static const Gtk::TreeModelColumn<Glib::ustring>& getFilenameColumn() { return getInstance().filename; }
 private:
   ThreadDirColumns() { add(filename); };
   ThreadDirColumns(const ThreadDirColumns&) = delete;
   void operator=(const ThreadDirColumns&) = delete;
 };

  class Application {
  public:
    explicit Application(int argc, char *argv[]);
    void run();

  private:
    const int LURKER_TIMEOUT_SECS;

    void signal_go();
    void download_finished();
    void downloads_finished(int num, const Request& request);
    void starting_downloads(int num);
    void download_error(const Derp::Error&);
    void update_progressBar();

    void update_thread_dir_completer();
    void update_thread_dir_finish(const Glib::RefPtr<Gio::AsyncResult>& result);
    void on_url_entry(guint, const gchar*, guint);
    void on_board_toggled();
    void on_thread_toggled();

	  sigc::connection progressConnection;
    int num_downloading;
    int num_downloaded;
    int num_download_errors;

    Gtk::Main m_kit;

    Glib::Timer m_timer;
    Derp::Manager m_manager;
    Derp::Lurker m_lurker;

    Gtk::Window* m_window;
    Gtk::Image* m_image;
    Gtk::ProgressBar* m_progressBar;
    Gtk::ToggleButton* m_goButton;
    Gtk::SpinButton* m_lurkSpinButton;
    Gtk::Entry* m_urlEntry;
    Gtk::Entry* m_threadFolderEntry;
    Gtk::Label* m_threadLabel;
    Gtk::FileChooserButton* m_fileChooserButton;
    Gtk::CheckButton* m_boardDirCheckbox;
    Gtk::CheckButton* m_threadDirCheckbox;
    Gtk::CheckButton* m_originalFilenameCheckbox;
    Gtk::CheckButton* m_lurk404Checkbox;
    Gtk::Grid* m_headerGrid;
    Glib::RefPtr<Gdk::PixbufAnimation> m_killmegif;
    Glib::RefPtr<Gdk::PixbufAnimation> m_errorgif;
    Glib::RefPtr<Gdk::Pixbuf> m_fangpng;
    Glib::RefPtr<Gtk::Adjustment> m_lurkAdjustment;
    Glib::RefPtr<Gtk::Adjustment> m_xAdjustment;
    Glib::RefPtr<Gtk::Adjustment> m_yAdjustment;
    Glib::RefPtr<Gtk::EntryCompletion> m_entryCompletion;
    Glib::RefPtr<Gtk::ListStore> m_threadListStore;
  };
}
#endif
