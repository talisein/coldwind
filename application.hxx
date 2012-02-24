// Derp
#ifndef APPLICATION_HXX
#define APPLICATION_HXX
#include <gtkmm/main.h>
#include <glibmm/dispatcher.h>
#include <glibmm/thread.h>
#include <glibmm/refptr.h>
#include <gtkmm/image.h>
#include <gdkmm/pixbufanimation.h>
#include <gtkmm.h>
#include <glibmm/timer.h>
#include "parser.hxx"
#include "hasher.hxx"
#include "downloader.hxx"
#include "lurker.hxx"

namespace Derp {
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
    void download_error();
    void update_progressBar();

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
    Gtk::FileChooserButton* m_fileChooserButton;
    Gtk::CheckButton* m_boardDirCheckbox;
    Gtk::CheckButton* m_threadDirCheckbox;
    Gtk::CheckButton* m_originalFilenameCheckbox;
    Glib::RefPtr<Gdk::PixbufAnimation> m_killmegif;
    Glib::RefPtr<Gdk::PixbufAnimation> m_errorgif;
    Glib::RefPtr<Gdk::Pixbuf> m_fangpng;
    Glib::RefPtr<Gtk::Adjustment> m_lurkAdjustment;
    Glib::RefPtr<Gtk::Adjustment> m_xAdjustment;
    Glib::RefPtr<Gtk::Adjustment> m_yAdjustment;
  };
}
#endif
