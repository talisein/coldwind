#ifndef HASHER_HXX
#define HASHER_HXX

#include <set>
#include <string>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threadpool.h>
#include <giomm/file.h>
#include "image.hxx"
#include "request.hxx"

namespace Derp {
  class Hasher {
  public:
    Hasher();
    void hash_async(const Derp::Request& request);
    Glib::Dispatcher signal_hashing_finished;

    bool is_downloaded(const Image& img) const;

  private:
    static const goffset MAXIMUM_FILESIZE = 5100 * 1000;
    Hasher& operator=(const Hasher&); // Evil func
    Hasher(const Hasher&); // Evil func

    void hash(const Derp::Request& request);
    void hash_directory(const Glib::RefPtr<Gio::File>& dir);
    void hash_file(const Glib::RefPtr<Gio::File>& file);

    /**
       We need to protect access to m_xxx_set while hashing is
       ongoing, as multiple threads will be updating it.
    **/
    void insert_image(const Derp::Image& image);
    void insert_filepath(const std::string& filepath);
    bool is_hashed(const std::string& path) const;
    mutable Glib::Mutex hash_mutex;
    mutable Glib::Mutex filepath_mutex;
    std::set<std::string> m_filepath_set;
    std::set<Derp::Image> m_image_set;
    Glib::ThreadPool m_threadPool;
  };
}

#endif
