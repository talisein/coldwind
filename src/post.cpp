#include "post.hpp"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <charconv>
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
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_board(gobj()));
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
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_comment(gobj()));
    }

    std::string Post::get_subject() const {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_subject(gobj()));
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
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_name(gobj()));
    }

    std::string Post::get_tripcode() const {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_tripcode(gobj()));
    }

    std::string Post::get_capcode() const {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_capcode(gobj()));
    }

    std::string Post::get_number() const {
        auto id = horizon_post_get_post_number(gobj());
        std::array<char, std::numeric_limits<decltype(id)>::digits10 + 1> buf;
        auto [p, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), id);
        if (ec != std::errc()) [[unlikely]] {
                std::cerr << "get_renamed_filename(): bad conv for " << id << ", buf is size " << buf.size() << std::endl;
            return std::to_string(id);
        }
        return std::string(buf.data(), p);
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
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_md5(gobj()));
    }

    namespace {
        struct GFreer {
            void operator()(guchar *str) {
                g_free(str);
            }
        };
    }

    std::string Post::get_hash_hex() const {
        auto hash = get_hash();
        if (hash.empty())
            return hash;

        gsize outlen;
        std::unique_ptr<guchar[], GFreer> binary(g_base64_decode(hash.c_str(), &outlen));
        std::string ret;
        ret.reserve(outlen*2);
        for (gsize i = 0; i < outlen; ++i) {
            char str[3];
            if (3 == g_snprintf(str, sizeof(str), "%.2x", binary[i])) {
                ret.append(str, 2);
            } else {
                ret.append("??");
            }
        }

        return ret;
    }

    std::string Post::get_thumb_url() {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_thumb_url(gobj()));
    }

    std::string Post::get_image_url() {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_image_url(gobj()));
}


    std::string Post::get_original_filename() const {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_original_filename(gobj()));
    }

    std::string Post::get_renamed_filename() const {
        auto name = horizon_post_get_renamed_filename(gobj());
        std::array<char, std::numeric_limits<decltype(name)>::digits10 + 1> buf;
        auto [p, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), name);
        if (ec != std::errc()) [[unlikely]] {
                std::cerr << "get_renamed_filename(): bad conv for " << name << ", buf is size " << buf.size() << std::endl;
            return std::to_string(name);
        }
        return std::string(buf.data(), p);
    }

    std::string Post::get_image_ext() const {
        return Glib::convert_const_gchar_ptr_to_stdstring(horizon_post_get_ext(gobj()));
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
        Glib::wrap_register(horizon_post_get_type(), &Post_Class::wrap_new);
    }
}

namespace Glib
{
    Glib::RefPtr<Derp::Post> wrap(HorizonPost *object, bool take_copy) {
        return Glib::RefPtr<Derp::Post>( dynamic_cast<Derp::Post*>( Glib::wrap_auto (G_OBJECT(object), take_copy)) );
    }
}
