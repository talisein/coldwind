#include <queue>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threadpool.h>

namespace Derp {
  class Downloader {
  public:
    Downloader();
    void download_async(const std::queue<Glib::ustring>& urls, const std::string& path);

    Glib::Dispatcher signal_download_finished;
  private:
    Downloader& operator=(const Downloader&); // Evil func
    Downloader(const Downloader&); // Evil func

    void download_urls(std::queue<Glib::ustring> urls, const std::string& path);
    void download_url(const Glib::ustring& url, std::string path);
    Glib::ThreadPool m_threadPool;
  };
}
