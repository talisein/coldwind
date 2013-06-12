#include "post.hpp"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <glibmm/datetime.h>

namespace Derp {
	const Glib::Class& Post_Class::init() {
		if (!gtype_) {
			class_init_func_ = &Post_Class::class_init_function;
			register_derived_type(horizon_post_get_type());
		}
		
		return *this;
	}

	void Post_Class::class_init_function(void *g_class, void *class_data) {
		BaseClassType *const klass = static_cast<BaseClassType*>(g_class);
		CppClassParent::class_init_function(klass, class_data);
	}

	Glib::ObjectBase* Post_Class::wrap_new(GObject *object) {
		return new Post((HorizonPost*) object);
	}

	HorizonPost* Post::gobj_copy() {
		reference();
		return gobj();
	}

	Post::Post(HorizonPost* castitem) :
		Glib::Object((GObject*) castitem)
	{}

	Post::Post(const Glib::ConstructParams &params) :
		Glib::Object(params)
	{}

	Post::CppClassType Post::post_class_;

	GType Post::get_type() {
		return post_class_.init().get_type();
	}

	GType Post::get_base_type() {
		return horizon_post_get_type();
	}

	Post::~Post() {
	}

	void Post::set_board(const std::string& in) {
		horizon_post_set_board(gobj(), in.c_str());
	}

	std::string Post::get_board() const {
		const gchar *board = horizon_post_get_board(gobj());
		std::stringstream s;

		if (board)
			s << board;
		
		return s.str();
	}

	gint64 Post::get_thread_id() const {
		return horizon_post_get_thread_id(gobj());
	}

	void Post::set_thread_id(const gint64 id) {
		horizon_post_set_thread_id(gobj(), id);
	}

	bool Post::is_same_post(const Glib::RefPtr<Post> &post) const {
		return horizon_post_is_same_post(gobj(), post->gobj());
	}

	bool Post::is_not_same_post(const Glib::RefPtr<Post> &post) const {
		return horizon_post_is_not_same_post(gobj(), post->gobj());
	}

	void Post::update(const Post&) {
		g_warning("Post update not implemented");
	}

	std::string Post::get_comment() const {
		const gchar *comment = horizon_post_get_comment(gobj());
		std::stringstream out;

		if (comment) {
			out << comment;
		} 

		return out.str();
	}

	std::string Post::get_subject() const {
		const gchar *subject = horizon_post_get_subject(gobj());
		std::stringstream out;

		if (subject) {
			out << subject;
		}

		return out.str();
	}
		
	Glib::ustring Post::get_time_str() const {
		gint64 ctime = horizon_post_get_time(gobj());
		Glib::ustring out;
		Glib::DateTime time = Glib::DateTime::create_now_local(ctime);
		if (G_LIKELY( ctime >= 0 )) {
			out = time.format("%A, %-l:%M:%S %P");
		} 

		return out;
	}

	std::string Post::get_name() const {
		const gchar* name = horizon_post_get_name(gobj());
		std::stringstream out;

		if (name) {
			out << name;
		}
		
		return out.str();
	}

	std::string Post::get_tripcode() const {
		const gchar* trip = horizon_post_get_tripcode(gobj());
		std::stringstream out;
		
		if (trip) {
			out << trip;
		}

		return out.str();
	}

	std::string Post::get_capcode() const {
		const gchar *capcode = horizon_post_get_capcode(gobj());
		std::stringstream out;

		if (capcode) {
			out << capcode;
		}

		return out.str();
	}

	std::string Post::get_number() const {
		gint64 id = horizon_post_get_post_number(gobj());
		std::stringstream out;

		if (id > 0) {
			out << id;
		}

		return out.str();
	}

	gint64 Post::get_id() const {
		return horizon_post_get_post_number(gobj());
	}

	gint64 Post::get_unix_time() const {
		return horizon_post_get_time(gobj());
	}

	gint64 Post::get_file_size() const {
		return horizon_post_get_fsize(gobj());
	}

	std::string Post::get_hash() const {
		const gchar *str = horizon_post_get_md5(gobj());
		std::stringstream out;

		if (str) {
			out << str;
		}

		return out.str();
	}

    namespace {
        struct GFreer {
            void operator()(guchar *str) {
                g_free(str);
            }
        };

        std::string
        base64_to_hex(const std::string& base64)
        {
            gsize outlen;
            std::string ret;
            std::unique_ptr<guchar[], GFreer> binary(g_base64_decode(base64.c_str(), &outlen));
            ret.reserve(outlen*2);
            for (gsize i = 0; i < outlen; ++i) {
                char str[3];
                g_snprintf(str, sizeof(str), "%.2x", binary[i]);
                ret.append(str, 2);
            }

            return ret;
        }
    }

    std::string Post::get_hash_hex() const {
        return base64_to_hex(get_hash());
    }

	std::string Post::get_thumb_url() {
		const gchar *url = horizon_post_get_thumb_url(gobj());
		std::stringstream out;

		if (url) {
			out << url;
		} 

		return out.str();
	}

	std::string Post::get_image_url() {
		const gchar *url = horizon_post_get_image_url(gobj());
		std::stringstream out;
		
		if (url) {
			out << url;
		}

		return out.str();
	}


	std::string Post::get_original_filename() const {
		const gchar *filename = horizon_post_get_original_filename(gobj());
		std::stringstream out;

		if (filename) {
			out << static_cast<const char*>(filename);
		}

		return out.str();
	}

    std::string Post::get_renamed_filename() const {
        auto filename = horizon_post_get_renamed_filename(gobj());

        return std::to_string(filename);
    }

	std::string Post::get_image_ext() const {
		const gchar *ext = horizon_post_get_ext(gobj());
		std::stringstream out;

		if (ext) {
			out << static_cast<const char*>(ext);
		}

		return out.str();
	}

	bool Post::is_gif() const {
		return static_cast<bool>(horizon_post_is_gif(gobj()));
	}

	gint Post::get_thumb_width() const {
		return horizon_post_get_thumbnail_width(gobj());
	}

	gint Post::get_thumb_height() const {
		return horizon_post_get_thumbnail_height(gobj());
	}

	gint Post::get_height() const {
		return horizon_post_get_height(gobj());
	}

	gint Post::get_width() const {
		return horizon_post_get_width(gobj());
	}

	std::size_t Post::get_fsize() const {
		return static_cast<std::size_t>(horizon_post_get_fsize(gobj()));
	}

	bool Post::has_image() const {
		return horizon_post_has_image(gobj());
	}

	bool Post::is_sticky() const {
		return static_cast<bool>(horizon_post_get_sticky(gobj()));
	}

	bool Post::is_closed() const {
		return static_cast<bool>(horizon_post_get_closed(gobj()));
	}

	bool Post::is_deleted() const {
		return static_cast<bool>(horizon_post_get_deleted(gobj()));
	}

	bool Post::is_spoiler() const {
		return static_cast<bool>(horizon_post_get_spoiler(gobj()));
	}

	bool Post::is_rendered() const {
		return static_cast<bool>(horizon_post_is_rendered(gobj()));
	}

	void Post::mark_rendered() {
		horizon_post_set_rendered(gobj(), true);
	}

    gint Post::get_replies() const {
        return horizon_post_get_replies(gobj());
    }

    gint Post::get_images() const {
        return horizon_post_get_images(gobj());
    }

    bool Post::get_bumplimit() const {
        return static_cast<bool>(horizon_post_get_bumplimit(gobj()));
    }

    bool Post::get_imagelimit() const {
        return static_cast<bool>(horizon_post_get_imagelimit(gobj()));
    }

	void wrap_init() {
		Glib::wrap_register(horizon_post_get_type(), &Derp::Post_Class::wrap_new);
	}
}

namespace Glib 
{
	Glib::RefPtr<Derp::Post> wrap(HorizonPost *object, bool take_copy) {
		return Glib::RefPtr<Derp::Post>( dynamic_cast<Derp::Post*>( Glib::wrap_auto ((GObject*)object, take_copy)) );
	}
}
