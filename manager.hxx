#ifndef MANAGER_HXX
#define MANAGER_HXX
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include "parser.hxx"
#include "hasher.hxx"
#include "downloader.hxx"

namespace Derp {
  struct Lurk_Data {
    Glib::ustring thread_url;
    Glib::RefPtr<Gio::File> target_directory;
    int minutes;
    int xDim;
    int yDim;
  };

  class Manager {
  public:
    Manager();

    bool download_async(const Derp::Lurk_Data& data);
    sigc::signal<void, int, const Derp::Lurk_Data&> signal_all_downloads_finished;
    sigc::signal<void> signal_download_finished;
    sigc::signal<void, int> signal_starting_downloads;

  private:

    void parsing_finished();
    void hashing_finished();
    void try_download();
    void download_finished();
    void done();

    bool is_working;
    bool is_hashing, is_parsing;
    int num_downloading, num_downloaded;

    Derp::Parser m_parser;
    Derp::Hasher m_hasher;
    Derp::Downloader m_downloader;

    Derp::Lurk_Data m_data;

  };
}

#endif
