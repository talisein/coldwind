#ifndef WINDOW_GTK3_HXX
#define WINDOW_GTK3_HXX

#include "window.hxx"
#include <gtkmm/treemodelcolumn.h>
#include <glibmm/ustring.h>
#include <gtkmm/window.h>
#include <gtkmm/image.h>
#include <gtkmm/progressbar.h>
#include <gdkmm/pixbufanimation.h>
#include <gtkmm/entrycompletion.h>
#include <gtkmm.h>
#include "config.h"


namespace Derp {
	class Window_Gtk3 final : public WindowImpl, public Gtk::Window {
	public:
		Window_Gtk3(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
		virtual ~Window_Gtk3();

		virtual void download_finished();
		virtual void downloads_finished(int num, const Request& request);
		virtual void starting_downloads(int num);
		virtual void download_error(const Derp::Error&);
		virtual void update_progress(double);
		virtual void run();

	private:
		sigc::signal<bool, const Request&> signal_go_;
		int num_downloading_;
		int num_downloaded_;
		int num_download_errors_;
		double progress_;

		void on_url_entry(guint, const gchar*, guint);
		void on_board_toggled();
		void on_thread_toggled();
		void goButton_click();
		void update_progressBar();
		
		virtual void on_hide();

		void update_thread_dir_completer();
        void update_thread_dir_foreach_info(const Glib::RefPtr<Gio::AsyncResult>&,
                                            Glib::RefPtr<Gio::FileEnumerator>&);
		Gtk::Image* image_;
		Gtk::ProgressBar* progressBar_;
		Gtk::ToggleButton* goButton_;
		Gtk::Entry* urlEntry_;
		Gtk::Entry* threadFolderEntry_;
		Gtk::Label* threadLabel_;
		Gtk::FileChooserButton* fileChooserButton_;
		Gtk::CheckButton* boardDirCheckbox_;
		Gtk::CheckButton* threadDirCheckbox_;
		Gtk::CheckButton* originalFilenameCheckbox_;
		Gtk::CheckButton* lurk404Checkbox_;
		Gtk::Grid* headerGrid_;
		Glib::RefPtr<Gdk::PixbufAnimation> killmegif_;
		Glib::RefPtr<Gdk::PixbufAnimation> errorgif_;
		Glib::RefPtr<Gdk::Pixbuf> fangpng_;
		Glib::RefPtr<Gtk::Adjustment> lurkAdjustment_;
		Glib::RefPtr<Gtk::Adjustment> xAdjustment_;
		Glib::RefPtr<Gtk::Adjustment> yAdjustment_;

        class ThreadDirColumns : public Gtk::TreeModel::ColumnRecord {
        public:
            Gtk::TreeModelColumn<Glib::ustring> filename;
            ThreadDirColumns() { add(filename); };
        } columns_;
		Glib::RefPtr<Gtk::EntryCompletion> entryCompletion_;
		Glib::RefPtr<Gtk::ListStore> threadListStore_;
        Glib::RefPtr<Gio::Cancellable> completion_cancellable_;

	};

}


#endif
