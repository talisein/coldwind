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
#include <gtkmm/treeview.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/infobar.h>
#include <gtkmm.h>
#include "config.h"


namespace Derp {
	class Window_Gtk3 final : public Gtk::Window, public WindowImpl {
	public:
		Window_Gtk3(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
		virtual ~Window_Gtk3();

		virtual void run();

	private:
		sigc::signal<bool, const Request&> signal_go_;

		void on_url_entry(guint, const gchar*, guint);
		void on_board_toggled();
		void on_thread_toggled();
        
        void activate_new_request();
        void activate_clear();

		virtual void on_hide();

		void update_thread_dir_completer();
        void update_thread_dir_foreach_info(const Glib::RefPtr<Gio::AsyncResult>&,
                                            Glib::RefPtr<Gio::FileEnumerator>&);

        virtual void request_changed_state    (const std::shared_ptr<const ManagerResult>& result) override;
        virtual void request_download_complete(const std::shared_ptr<const ManagerResult>& result) override;
        virtual void request_error            (const std::shared_ptr<const ManagerResult>& result) override;

        
		Gtk::FileChooserButton* fileChooserButton_;
		Gtk::CheckButton* boardDirCheckbox_;
		Gtk::CheckButton* threadDirCheckbox_;
		Gtk::CheckButton* originalFilenameCheckbox_;
		Gtk::CheckButton* lurk404Checkbox_;
        Gtk::Statusbar* status_bar_;
        Gtk::TreeView* request_tree_view_;
        Gtk::InfoBar* info_bar_;
		Gtk::Entry* urlEntry_;
		Gtk::Entry* threadFolderEntry_;
		Gtk::Label* threadLabel_;

		Glib::RefPtr<Gtk::Adjustment> lurkAdjustment_;
		Glib::RefPtr<Gtk::Adjustment> xAdjustment_;
		Glib::RefPtr<Gtk::Adjustment> yAdjustment_;

		Glib::RefPtr<Gtk::Action> clear_action_;
		Glib::RefPtr<Gtk::Action> new_request_action_;

        class ThreadDirColumns : public Gtk::TreeModel::ColumnRecord {
        public:
            Gtk::TreeModelColumn<Glib::ustring> filename;
            ThreadDirColumns() { add(filename); };
        } completion_columns_;
		Glib::RefPtr<Gtk::EntryCompletion> entryCompletion_;
		Glib::RefPtr<Gtk::ListStore> threadListStore_;
        Glib::RefPtr<Gio::Cancellable> completion_cancellable_;

        class RequestColumns : public Gtk::TreeModel::ColumnRecord {
        public:
            Gtk::TreeModelColumn<std::size_t> request_id;
            Gtk::TreeModelColumn<Glib::ustring> title;
            Gtk::TreeModelColumn<gint> replies;
            Gtk::TreeModelColumn<gint> images;
            Gtk::TreeModelColumn<std::size_t> fetching;
            Gtk::TreeModelColumn<std::size_t> fetched;
            Gtk::TreeModelColumn<std::size_t> errors;
            Gtk::TreeModelColumn<std::size_t> counted_errors;
            Gtk::TreeModelColumn<float> megabytes_fetched;
            Gtk::TreeModelColumn<Glib::ustring> state;
            RequestColumns() { add(request_id); add(title);
                add(replies); add(images); add(fetching);
                add(fetched); add(errors); add(counted_errors);
                add(megabytes_fetched); add(state); };
        } request_columns_;
        Glib::RefPtr<Gtk::ListStore> request_list_store_;
	};

}


#endif
