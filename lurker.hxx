#ifndef LURKER_HXX
#define LURKER_HXX
#include <glibmm/thread.h>
#include "parser.hxx"
#include "hasher.hxx"
#include "downloader.hxx"

namespace Derp {
  struct Lurk_Data {
    Glib::ustring thread_url;
    Glib::RefPtr<Gio::File> target_directory;
    int minutes;
  };

  class Lurker {
  public:
    Lurker();

    void add_async(const Derp::Lurk_Data& data);
    void run_async();

  private:
    Lurker(const Lurker&); // evil func
    const Lurker& operator=(const Lurker&); // evil func

    void add(const Derp::Lurk_Data&);
    Glib::Mutex m_list_lock;
    std::list<Derp::Lurk_Data> m_list;

    void parsing_finished();
    void hashing_finished();
    void try_download();
    void download_finished();
    bool is_hashing, is_parsing;
    int num_downloaded;

    Derp::Parser m_parser;
    Derp::Hasher m_hasher;
    Derp::Downloader m_downloader;

    void run();
  };
}

#endif
