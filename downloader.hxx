#ifndef DOWNLOADER_HXX
#define DOWNLOADER_HXX

#include <queue>
#include <list>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threadpool.h>
#include <curl/curl.h>
#include "image.hxx"
#include <glibmm/iochannel.h>
#include <giomm/file.h>

namespace Derp {
  struct Socket_Info {
    Glib::RefPtr<Glib::IOChannel> channel;
    curl_socket_t s;
    int action;
    CURL* curl;
    sigc::connection connection;
  };

  class Downloader {
  public:
    Downloader();
    ~Downloader();

    void download_async(const std::list<Derp::Image>& imgs, const Glib::RefPtr<Gio::File>& p_dir);
    void download_imgs_multi(const std::list<Derp::Image>& imgs, const Glib::RefPtr<Gio::File>& p_dir);

    Glib::Dispatcher signal_download_finished;
    Glib::Dispatcher signal_download_error;
  private:
    Downloader& operator=(const Downloader&); // Evil func
    Downloader(const Downloader&); // Evil func

    void download_imgs(const std::list<Derp::Image>& imgs, const Glib::RefPtr<Gio::File>& p_dir);
    void download_url(const Glib::ustring& url, const Glib::RefPtr<Gio::File>&);
    bool curl_setup(CURL* curl, const Derp::Image& img);

    Glib::ThreadPool m_threadPool;

    CURLM* m_curlm;
    sigc::connection m_timeout_connection;
    int m_running_handles;
    std::map<CURL*, Glib::RefPtr<Gio::FileOutputStream>> m_fos_map;
    std::list<Derp::Image> m_imgs;
    Glib::RefPtr<Gio::File> m_target_dir;

    bool curl_timeout_expired_cb();
    void curl_check_info();
    void curl_addsock(curl_socket_t s, CURL *easy, int action);
    void curl_setsock(Socket_Info* info, curl_socket_t s, CURL* curl, int action);
    void curl_remsock(Derp::Socket_Info* info);
    bool curl_event_cb(Glib::IOCondition condition, curl_socket_t s);

    //    void curl_remsock(
    friend int curl_socket_cb(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
    friend int curl_timer_cb(CURLM *multi, long timeout_ms, void *userp);
    friend size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);
  };

  int curl_socket_cb(CURL *easy, /* easy handle */   
		     curl_socket_t s, /* socket */   
		     int action, /* see values below */   
		     void *userp, /* private callback pointer */   
		     void *socketp); /* private socket pointer */

  int curl_timer_cb(CURLM *multi,    /* multi handle */
		    long timeout_ms, /* see above */
		    void *userp);    /* private callback
					pointer */

  size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);
}


#endif
