#include "window_gtk3.hxx"
#include <iostream>
#include <gdkmm/pixbufanimation.h>
#include "post.hpp"
#include <gtkmm/button.h>

/*
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
*/

namespace sigc {
    SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE
}

namespace {
    using namespace std::string_literals;
    const std::vector<std::string> FEH_ARGV {"/usr/bin/feh"s, "-FZ"s, "."s};
}

Derp::Window_Gtk3::Window_Gtk3(BaseObjectType* cobject,
                               const Glib::RefPtr<Gtk::Builder>& refBuilder)
	: Gtk::Window(cobject),
	  Derp::WindowImpl(),
      entryCompletion_(Gtk::EntryCompletion::create()),
      threadListStore_(Gtk::ListStore::create(completion_columns_)),
      completion_cancellable_(Gio::Cancellable::create()),
      request_list_store_(Gtk::ListStore::create(request_columns_))
{
    Gtk::Button *new_request_button;
    Gtk::Grid *headerGrid;

    refBuilder->get_widget("urlEntry", urlEntry_);
    refBuilder->get_widget("threadFolderEntry", threadFolderEntry_);
    refBuilder->get_widget("fileChooserButton", fileChooserButton_);
    refBuilder->get_widget("boardDirCheckbox", boardDirCheckbox_);
    refBuilder->get_widget("threadDirCheckbox", threadDirCheckbox_);
    refBuilder->get_widget("originalFilenameCheckbox", originalFilenameCheckbox_);
    refBuilder->get_widget("lurk404Checkbox", lurk404Checkbox_);
    refBuilder->get_widget("threadLabel", threadLabel_);
    refBuilder->get_widget("request_tree_view", request_tree_view_);
    refBuilder->get_widget("info_bar", info_bar_);
    refBuilder->get_widget("status_bar", status_bar_);
    refBuilder->get_widget("new_request_button", new_request_button);

    fileChooserButton_->set_current_folder(Glib::get_user_special_dir(Glib::UserDirectory::USER_DIRECTORY_DESKTOP));

    lurkAdjustment_ = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("lurk_adjustment"));
    xAdjustment_    = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("width_adjustment"));
    yAdjustment_    = Glib::RefPtr<Gtk::Adjustment>::cast_static(refBuilder->get_object("height_adjustment"));

    clear_action_       = Glib::RefPtr<Gtk::Action>::cast_static(refBuilder->get_object("clear_done_action"));
    clear_action_->signal_activate().connect(sigc::mem_fun(*this, &Window_Gtk3::activate_clear));
    new_request_action_ = Glib::RefPtr<Gtk::Action>::cast_static(refBuilder->get_object("new_request_action"));
    new_request_action_->signal_activate().connect(sigc::mem_fun(*this, &Window_Gtk3::activate_new_request));

    refBuilder->get_widget("primary_control_grid", headerGrid);
    headerGrid->set_focus_chain({urlEntry_, fileChooserButton_, threadFolderEntry_, new_request_button});

    entryCompletion_->set_model(threadListStore_);
    entryCompletion_->set_text_column(completion_columns_.filename);
    entryCompletion_->set_inline_completion(true);
    entryCompletion_->set_inline_selection(false);
    entryCompletion_->set_popup_completion(true);
    threadFolderEntry_->set_completion(entryCompletion_);

    auto renderer = Gtk::manage(new Gtk::CellRendererText());
    renderer->property_ellipsize().set_value(Pango::ELLIPSIZE_END);
    renderer->property_ellipsize_set().set_value(true);
    auto column = Gtk::manage(new Gtk::TreeViewColumn("Thread", *renderer));
    column->add_attribute(*renderer, "text", request_columns_.title);
    column->set_sizing(Gtk::TREE_VIEW_COLUMN_GROW_ONLY);
    column->set_resizable(true);
    column->set_expand(true);

    request_tree_view_->set_model(request_list_store_);
    request_tree_view_->append_column(*column);
    request_tree_view_->append_column("Replies", request_columns_.replies);
    request_tree_view_->append_column("Images", request_columns_.images);
    request_tree_view_->append_column("New", request_columns_.fetching);
    request_tree_view_->append_column("Got", request_columns_.fetched);
    request_tree_view_->append_column("Failed", request_columns_.errors);
    request_tree_view_->append_column_numeric("MiB", request_columns_.megabytes_fetched, "%0.2f");
    request_tree_view_->append_column("State", request_columns_.state);
    request_tree_view_->signal_row_activated().connect([&](const Gtk::TreePath &path, Gtk::TreeViewColumn *column) -> void{
        if (!path)
            return;
        auto iter = request_list_store_->get_iter(path);
        auto req = iter->get_value(request_columns_.req);
        try {
            Glib::spawn_async(req.getDirectory()->get_path(), FEH_ARGV);
        } catch (Glib::SpawnError &e) {
            std::cerr << "failed to launch feh: " << e.what() << "\n";
            return;
        }
    });


    urlEntry_->get_buffer()->signal_inserted_text().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_url_entry) );
    boardDirCheckbox_->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_board_toggled) );
    threadDirCheckbox_->signal_toggled().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_thread_toggled) );
    fileChooserButton_->signal_file_set().connect( sigc::mem_fun(*this, &Derp::Window_Gtk3::on_board_toggled) );

    on_thread_toggled(); // Show/Hide the entryCompletion based on xml
}

Derp::Window_Gtk3::~Window_Gtk3() = default;

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
/*
namespace {
    extern "C" {
        struct _foreach_info_data {
            Glib::RefPtr<Gtk::ListStore> *store;
            Gtk::TreeModelColumn<Glib::ustring> *filename;
        };

        void _foreach_info(gpointer data, gpointer user_data) {
            _foreach_info_data *d = static_cast<_foreach_info_data*>(user_data);
            GFileInfo *info = static_cast<GFileInfo*>(data);
            (*(d->store))->append()->set_value(*(d->filename), Glib::convert_const_gchar_ptr_to_ustring(g_file_info_get_display_name(info)));
        }
    }
}
*/
void Derp::Window_Gtk3::update_thread_dir_foreach_info(const Glib::RefPtr<Gio::AsyncResult>& result)
{
/*    auto info_list = enumerator->next_files_finish(result);
    if (!info_list.empty()) {
        GList *l = info_list.data();
        _foreach_info_data d {&threadListStore_, &completion_columns_.filename};
        g_list_foreach(l, _foreach_info, &d);


//        for ( const auto & info : info_list ) {
//            if (info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY) {
//                threadListStore_->append()->set_value(completion_columns_.filename, Glib::ustring(info->get_display_name()));
//            }

        enumerator->close_async(Glib::PRIORITY_DEFAULT, [this](Glib::RefPtr<Gio::AsyncResult>& result) {
                enumerator->close_finish(result);
            });
    }*/
}

void Derp::Window_Gtk3::update_thread_dir_completer() {
    if (!completion_cancellable_->is_cancelled())
        completion_cancellable_->cancel();
    if (!threadDirCheckbox_->get_active()) {
        threadListStore_->clear();
        return;
    }

    Glib::RefPtr<Gio::File> basedir = fileChooserButton_->get_file();
    if (!basedir)
        return;
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

    /*
    completion_cancellable_ = Gio::Cancellable::create();
    dir->enumerate_children_async(
        [this, dir](const Glib::RefPtr<Gio::AsyncResult>& result) {
            enumerator = dir->enumerate_children_finish(result);
            if (!enumerator)
                return;
            enumerator->next_files_async(
                [this] (const Glib::RefPtr<Gio::AsyncResult>& res) {
                    update_thread_dir_foreach_info(res);
                });
        },
        completion_cancellable_, "standard::type,standard::display-name");
           */
}

void Derp::Window_Gtk3::run() {
	show_all();
    info_bar_->hide();
}

void Derp::Window_Gtk3::on_hide() {
	Gtk::Window::on_hide();
}

namespace Derp
{
    void
    Window_Gtk3::activate_new_request()
    {
        Request request(urlEntry_->get_text(),
                        fileChooserButton_->get_file(),
                        threadFolderEntry_->get_text(),
                        static_cast<int>(lurkAdjustment_->get_value()),
                        static_cast<int>(xAdjustment_->get_value()),
                        static_cast<int>(yAdjustment_->get_value()),
                        boardDirCheckbox_->get_active(),
                        threadDirCheckbox_->get_active(),
                        originalFilenameCheckbox_->get_active(),
                        lurk404Checkbox_->get_active());
        signal_new_request(request);
    }

    void
    Window_Gtk3::activate_clear()
    {
        auto iter = request_list_store_->children().begin();
        while (iter) {
            if (iter->get_value(request_columns_.state) == "Done") {
                iter = request_list_store_->erase(iter);
            } else {
                ++iter;
            }
        }
    }

    void
    Window_Gtk3::request_changed_state (const std::shared_ptr<const ManagerResult>& result)
    {
        bool found_iter = false;
        Gtk::TreeModel::iterator iter;
        auto const id = result->request.get_request_id();
        Glib::ustring title;
        std::stringstream ss;

        request_list_store_->foreach_iter([this, id, &result, &found_iter, &iter]
                                          (const Gtk::TreeModel::iterator& i)
                                          -> bool {
                if (id == i->get_value(request_columns_.request_id)) {
                    iter = i;
                    found_iter = true;
                    return true;
                }
                return false;
            });
        if (!found_iter) {
            iter = request_list_store_->append();
            ss << "/" << result->request.getBoard() << "/" << result->request.getThread();
            title = ss.str();
            iter->set_value(request_columns_.title, title);
            iter->set_value(request_columns_.request_id, result->request.get_request_id());
        }
        iter->set_value(request_columns_.req, result->request);

        std::size_t num_downloading;
        std::size_t counted_errors;
        std::size_t errors;
        std::string subject;
        gint images;
        switch (result->state) {
            case ManagerResult::HASHING:
                iter->set_value(request_columns_.state, Glib::ustring("Hashing"));
                break;
            case ManagerResult::PARSING:
                iter->set_value(request_columns_.state, Glib::ustring("Parsing"));
                break;
            case ManagerResult::DOWNLOADING:
                iter->set_value(request_columns_.state, Glib::ustring("Downloading"));
                errors = iter->get_value(request_columns_.errors);
                counted_errors = iter->get_value(request_columns_.counted_errors);
                num_downloading = iter->get_value(request_columns_.fetching);
                num_downloading += result->num_downloading - (errors - counted_errors);
                iter->set_value(request_columns_.counted_errors, errors);
                iter->set_value(request_columns_.fetching, num_downloading);
                if (result->op) {
                    images = result->op->get_images();
                    images += (result->op->has_image()&&!result->op->is_deleted())?1:0;
                    iter->set_value(request_columns_.replies, result->op->get_replies());
                    iter->set_value(request_columns_.images, images);
                    subject = result->op->get_subject();
                    if (subject.size() > 0) {
                        ss << subject << " (/" << result->request.getBoard() << "/" << result->request.getThread() << ")";
                        title = ss.str();
                        iter->set_value(request_columns_.title, title);
                    }
                }
                break;
            case ManagerResult::LURKING:
                iter->set_value(request_columns_.state, Glib::ustring("Lurking"));
                break;
            case ManagerResult::DONE:
                iter->set_value(request_columns_.state, Glib::ustring("Done"));
                break;
            case ManagerResult::ERROR:
                iter->set_value(request_columns_.state, Glib::ustring("Error"));
                break;
            default:
                break;
        }
    }

    void
    Window_Gtk3::request_download_complete (const std::shared_ptr<const ManagerResult>& result)
    {
        request_list_store_->foreach_iter([this, &result](const Gtk::TreeModel::iterator& iter)->bool{
                auto id = result->request.get_request_id();
                if (id == iter->get_value(request_columns_.request_id)) {
                    if (result->had_error) {
                        auto errors = iter->get_value(request_columns_.errors);
                        iter->set_value(request_columns_.errors, ++errors);
                        std::cerr << "Error: " << result->info.error_str << std::endl;
                    } else {
                        auto downloaded = iter->get_value(request_columns_.fetched);
                        ++downloaded;
                        iter->set_value(request_columns_.fetched, downloaded);
                        auto bytes = iter->get_value(request_columns_.megabytes_fetched);
                        bytes += (result->info.size / (1024. * 1024.));
                        iter->set_value(request_columns_.megabytes_fetched, bytes);
                    }
                    return true;
                } else {
                    return false;
                }
            });
    }

    void
    Window_Gtk3::request_error (const std::shared_ptr<const ManagerResult>& result)
    {
        std::cerr << "Error: " << result->error_str << std::endl;
    }
}
