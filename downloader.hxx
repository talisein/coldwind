#ifndef DOWNLOADER_HXX
#define DOWNLOADER_HXX

#include <queue>
#include <list>
#include <set>
#include <curl/curl.h>
#include <giomm/file.h>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threadpool.h>
#include <glibmm/iochannel.h>
#include <glibmm/timer.h>
#include "image.hxx"
#include "request.hxx"

namespace Derp {
	class Downloader;

	struct Socket_Info {
		Glib::RefPtr<Glib::IOChannel> channel;
		sigc::connection connection;
	};

	struct Progress_Info {
		Derp::Downloader* const downloader;
		double last;
		const double expected;
	};

	const int COLDWIND_CURL_CONNECTIONS = 5;

	class Downloader {
	public:
		Downloader();
		~Downloader();

		void download_async_easy(const std::list<Derp::Image>&, const Derp::Request&);
		void download_async(const std::list<Derp::Image>&, const Derp::Request&);

		Glib::Dispatcher signal_download_finished;
		Glib::Dispatcher signal_download_error;

		double getProgress() const { return progress_bytes_ / expected_bytes_; };
	private:
		Downloader& operator=(const Downloader&) = delete; // Evil func
		Downloader(const Downloader&) = delete; // Evil func

		void download_imgs(const std::list<Derp::Image>& imgs, const Derp::Request&);
		void download_url(const Glib::ustring& url, const Derp::Request&);

		Glib::ThreadPool m_threadPool;

		mutable Glib::Mutex curl_mutex;
		CURLM* m_curlm;
		std::queue<CURL*> m_curl_queue;
		sigc::connection m_timeout_connection;
		int m_running_handles;
		volatile double progress_bytes_;
		double expected_bytes_;

		std::map<CURL*, Glib::RefPtr<Gio::FileOutputStream>> m_fos_map;
		std::map<CURL*, Glib::RefPtr<Gio::File>> m_file_map;

		std::list<Derp::Image> m_imgs;
		Glib::RefPtr<Gio::File> m_target_dir;
		Derp::Request m_request;
		double m_total_bytes;
		Glib::Timer m_timer;

		std::list<curl_socket_t> active_sockets_;
		std::vector<Socket_Info*> socket_info_cache_;
		std::list<Socket_Info*> active_socket_infos_;
		std::vector<Socket_Info*> bad_socket_infos_;
		inline void ASSERT_LOCK(const std::string& func) const;
		void collect_statistics(CURL* curl);
		bool finish_file_operations(CURL* curl, bool hasDownloadError);
		void start_new_download(CURL* curl);
		void increment_progress(double progress) {progress_bytes_ += progress;};

		bool curl_setup(CURL* curl, const Derp::Image& img);
		void curl_check_info();
		void curl_addsock(curl_socket_t s, CURL *easy, int action);
		void curl_setsock(Socket_Info* info, curl_socket_t s, CURL* curl, int action);
		void curl_remsock(Derp::Socket_Info* info, curl_socket_t s);
		bool curl_timeout_expired_cb();
		void curl_timeout_expired();
		bool curl_event_cb(Glib::IOCondition condition, curl_socket_t s);
		void curl_event(int action, curl_socket_t s);
		void download_imgs_multi();

		friend int curl_socket_cb(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);
		friend int curl_timer_cb(CURLM *multi, long timeout_ms, void *userp);
		friend size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);
		friend int downloader_progress_callback(void*, double, double, double, double);
	};

	int downloader_progress_callback(void*, double, double, double, double);

	int curl_socket_cb(CURL *easy,      /* easy handle */   
	                   curl_socket_t s, /* socket */   
	                   int action,      /* see values below */   
	                   void *userp,     /* private callback pointer */   
	                   void *socketp);  /* private socket pointer */

	int curl_timer_cb(CURLM *multi,    /* multi handle */
	                  long timeout_ms, /* see above */
	                  void *userp);    /* private callback
	                                      pointer */
	size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);
}


#endif
