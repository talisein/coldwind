#ifndef DOWNLOADER_HXX
#define DOWNLOADER_HXX

#include <memory>
#include <queue>
#include <mutex>
#include <list>
#include <map>
#include <curl/curl.h>
#include <giomm/file.h>
#include <glibmm/ustring.h>
#include <glibmm/dispatcher.h>
#include <glibmm/threads.h>
#include "image.hxx"
#include "request.hxx"
#include "message_dispatcher.hpp"
#include "active.hpp"

namespace Derp {
    
    /** Statistical information for download.
     */
    struct DownloadInfo {
        DownloadInfo() = default;
        std::string url;      /* The url that was attempted */
        std::string filename; /* The actual filename saved to, if saved. */
        bool had_error;       /* True if an error occured */
        double total_time;    /* Time in seconds */
        double size;          /* bytes */
        double speed;         /* bytes per second */
    };

    /** libcurl wrapper to share cookies/ssl sessions between
     * CurlEasy. It is safe to use this share between CurlEasys in
     * separate threads.
     */
    class CurlShare {
        friend class CurlEasy;
        struct CURLSHDeleter {
            void operator()(CURLSH* curl) const {
                auto const code = curl_share_cleanup(curl);
                if (G_UNLIKELY(code != CURLSHE_OK)) {
                    std::stringstream ss;
                    ss << "Error: Unable to cleanup curl share: " << curl_share_strerror(code);
                    g_error(ss.str().c_str());
                }
            }
        };
    public:
        CurlShare();
        void lock(CURL* curl, curl_lock_data data, curl_lock_access);
        void unlock(CURL* curl, curl_lock_data data);

    private:
        std::map<curl_lock_data, std::unique_ptr<Glib::Threads::RWLock>> m_mutex_map;
        std::map<CURL*, std::map<curl_lock_data, std::unique_ptr<Glib::Threads::RWLock::ReaderLock>>> m_reader_map;
        std::map<CURL*, std::map<curl_lock_data, std::unique_ptr<Glib::Threads::RWLock::WriterLock>>> m_writer_map;
        std::unique_ptr<CURLSH, CURLSHDeleter> m_share;
    };

    /** libcurl wrapper that performs downloads in a persistent
     * dedicated thread. Provides results on GMainLoop.
     *
     */
    class CurlEasy {
    private:
        struct CURLDeleter {
            void operator()(CURL* curl) const {
                curl_easy_cleanup( curl );
            }
        };

    public:
        typedef std::pair<std::string, DownloadInfo> ResultPair;

        CurlEasy(const std::shared_ptr<CurlShare>& share);

        /** Attempts to fetch URL. download_complete is raised when
         * finished.
         *
         * If an error occurs, the error string will be raised on
         * signal_error and DownloadInfo::had_error will be true.
         */
        void download_async(const std::string& url);

        MessageDispatcher<ResultPair> download_complete;
        MessageDispatcher<std::string> signal_error;

    private:
        CurlEasy(const CurlEasy&) = delete;
        CurlEasy& operator=(const CurlEasy&) = delete;

        bool setup(const std::string& url);
        void download(const std::string& url);

        std::unique_ptr<char[]> m_error_buffer;
        std::unique_ptr<std::string> m_buffer;
        std::shared_ptr<CurlShare> m_share;
        std::unique_ptr<CURL, CURLDeleter> m_curl;
        Active active;
    };

	constexpr int COLDWIND_CURL_CONNECTIONS = 5;

    /** libcurl wrapper that performs multiple downloads in separate
     * threads. Callbacks are delivered on the GMainLoop thread.
     *
     * Note: This does not actually utilize libcurl's multi interface.
     */
    class CurlMulti {
    public:
        CurlMulti(const int max_connections = COLDWIND_CURL_CONNECTIONS);
        typedef std::function<void (std::string&& result, DownloadInfo&&)> CurlMultiCallback;

        /** Fetchs url and returns result in the callback.
         *
         * If an error occurs, the error string will be raised on
         * signal_error and DownloadInfo::had_error will be true.
         */
        void download_url_async(const std::string& url,
                                const CurlMultiCallback& cb);
        sigc::signal<void, const std::string&> signal_error;

    private:
        void start_download_from_queue();
        void on_download_completed(CurlEasy& curl);
        void on_download_error(CurlEasy& curl);

        std::queue<std::unique_ptr<CurlEasy>> curl_queue;
        std::queue<std::string> request_queue;
        std::deque<std::unique_ptr<CurlEasy>> active_curls;
        std::map<std::string, CurlMultiCallback> callback_map;
    };

    /** Downloads url to filename in target_dir.
     *
     * If filename already exists, a noncolliding name will be found
     * such as filename (2).ext
     *
     * All network and file operations are done off the main thread.
     */
	class Downloader {
	public:
		Downloader();
        /** Downloads url and saved to filename in target_dir.
         *
         * If filename already exists, a noncolliding filename will be
         * found such as filename (2).ext.
         */
        void download_async(const std::string& url,
                            const Glib::RefPtr<Gio::File>& target_dir,
                            const std::string& filename);

        /** Downloads url and passes the result back to the given
         * callback on the GMainLoop.
         *
         * Note: This method will not result in
         * signal_download_complete, but error strings will be fed
         * through signal_error.
         */
        void download_async(const std::string& url, const CurlMulti::CurlMultiCallback& cb);

        sigc::signal<void, const std::string&> signal_error;
        sigc::signal<void, const DownloadInfo&> signal_download_complete;


        /* Depreciated methods */
		void download_async(const std::list<Derp::Image>&, const Derp::Request&) __attribute__((deprecated));

        /* Depreciated signals */
		Glib::Dispatcher signal_download_finished;
		Glib::Dispatcher signal_download_error;

	private:
		Downloader& operator=(const Downloader&) = delete; // Evil func
		Downloader(const Downloader&) = delete; // Evil func

        MessageDispatcher<std::string> signal_internal_error;
        MessageDispatcher<DownloadInfo> signal_internal_download_complete;
        CurlMulti m_curl_multi;
        Active active;

        /** Callback from CurlMulti when curl has fetched url. Called
         * on GMainLoop.
         */
        void download_complete_cb(std::string&& data,
                                  DownloadInfo&& info, 
                                  const Glib::RefPtr<Gio::File>& target_dir,
                                  const std::string& filename);

        /** Writes data to disk. Intented to be performed in a
         * dedicated thread.
         */
        void download_write(const std::string& data, DownloadInfo& info,
                            const Glib::RefPtr<Gio::File>& target_dir,
                            const std::string& filename);
	};
}

#endif
