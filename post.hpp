#pragma once
#include <string>
#include <list>
#include <glibmm/datetime.h>
#include <glib.h>
#include <memory>
#include <functional>
#include <glibmm/dispatcher.h>
#include <glibmm/object.h>
#include <glibmm/private/object_p.h>
#include <glibmm/class.h>

extern "C" {
#include "horizon_post.h"
}

namespace Derp {
	class Post;

	class Post_Class : public Glib::Class {
	public:
		typedef Post CppObjectType;
		typedef HorizonPost BaseObjectType;
		typedef HorizonPostClass BaseClassType;
		typedef Glib::Object_Class CppClassParent;
		typedef GObjectClass BaseClassParent;

		friend class Post;

		const Glib::Class& init();
		static void class_init_function(void *g_class, void *class_data);
		static Glib::ObjectBase* wrap_new(GObject *object);
	};

	class Post : public Glib::Object {
	public:
		typedef Post CppObjectType;
		typedef Post_Class CppClassType;
		typedef HorizonPost BaseObjectType;
		typedef HorizonPostClass BaseObjectClassType;

	private: friend class Post_Class;
		static CppClassType post_class_;

	public:

		virtual ~Post();
		Post() = delete;
		explicit Post(const Post& in) = delete;
		Post& operator=(const Post& in) = delete;

		static GType get_type()   G_GNUC_CONST;
		static GType get_base_type()   G_GNUC_CONST;
		
		HorizonPost* gobj() {return reinterpret_cast<HorizonPost*>(gobject_);};
		const HorizonPost* gobj() const { return HORIZON_POST(gobject_);};
		HorizonPost* gobj_copy();

	protected:
		explicit Post(const Glib::ConstructParams& construct_params);
		explicit Post(HorizonPost* castitem);

	public:

		bool is_same_post(const Glib::RefPtr<Post> &post) const;
		bool is_not_same_post(const Glib::RefPtr<Post> &post) const;

		void update(const Post &in);
		void mark_rendered();
		bool is_rendered() const;

		std::string get_comment() const;
		gint64 get_id() const;
		gint64 get_unix_time() const;
		gint64 get_file_size() const;
		std::string get_subject() const;
		Glib::ustring get_time_str() const;
		std::string get_number() const;
		std::string get_name() const;
		std::string get_tripcode() const;
		std::string get_capcode() const;
		std::string get_original_filename() const;
        std::string get_renamed_filename() const;
		std::string get_image_ext() const;
		std::string get_hash() const;
        std::string get_hash_hex() const;
		std::string get_thumb_url();
		std::string get_image_url();
		gint get_thumb_width() const;
		gint get_thumb_height() const;
		gint get_width() const;
		gint get_height() const;
		std::size_t get_fsize() const;
		bool has_image() const;
		bool is_sticky() const;
		bool is_closed() const;
		bool is_deleted() const;
		bool is_spoiler() const;
		bool is_gif() const;
		void set_board(const std::string& board);
		std::string get_board() const;
		void set_thread_id(const gint64 id);
		gint64 get_thread_id() const;
	};

	void wrap_init();
}

namespace Glib
{
	Glib::RefPtr<Derp::Post> wrap(HorizonPost* object, bool take_copy = false);
}
