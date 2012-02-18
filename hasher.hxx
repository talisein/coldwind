#include <glib.h>
#include <set>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threadpool.h>
#include <giomm/file.h>
#include <functional>

namespace Derp {
  class Hasher {
  public:
    Hasher();
    void hash_async(const Glib::RefPtr<Gio::File>& dir);
    Glib::Dispatcher signal_hashing_finished;

    bool is_downloaded(const Glib::ustring& hash);

  private:
    Hasher& operator=(const Hasher&); // Evil func
    Hasher(const Hasher&); // Evil func

    void hash_directory(const Glib::RefPtr<Gio::File>& dir);
    void hash_file(const Glib::RefPtr<Gio::File>& file);

    /**
       We need to protect access to m_xxx_set while hashing is
       ongoing, as multiple threads will be updating it.
    **/
    void insert_hash(const Glib::ustring& hash);
    void insert_filepath(const std::string& filepath);
    bool is_hashed(const std::string& path);
    Glib::Mutex hash_mutex;
    Glib::Mutex filepath_mutex;
    std::set<Glib::ustring> m_hash_set;
    std::set<std::string> m_filepath_set;
    
    Glib::ThreadPool m_threadPool;
  };
}
