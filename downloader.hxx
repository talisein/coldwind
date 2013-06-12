#ifndef DOWNLOADER_HXX
#define DOWNLOADER_HXX

#include <memory>
#include <queue>
#include <deque>
#include <mutex>
#include <map>
#include <curl/curl.h>
#include <giomm/file.h>
#include <glibmm/threads.h>
#include "callback_dispatcher.hpp"
#include "active.hpp"

namespace Derp {
    
    /** Statistical information for download.
     */
    struct DownloadResult {
        DownloadResult() = default;
        std::string url;        /* The url that was attempted */
        std::string filename;   /* The actual filename saved to, if saved. */

        bool        had_error;  /* True if an error occured */
        long        error_code; /* HTTP Code E.g. 404 */
        std::string error_str;  /* Description of error */

        double      total_time; /* Time in seconds */
        double      size;       /* bytes */
        double      speed;      /* bytes per second */
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
        void lock(CURL* curl, curl_lock_data data, curl_lock_access access);
        void unlock(CURL* curl, curl_lock_data data);

    private:
        /* In C++14: std::mutex */
        typedef Glib::Threads::RWLock                     mutex_t;
        /* In C++14: std::shared_lock<std::mutex> */
        typedef Glib::Threads::RWLock::ReaderLock         reader_lock_t;
        /* In C++14: std::lock_guard<std::mutex> */
        typedef Glib::Threads::RWLock::WriterLock         writer_lock_t;

        typedef std::unique_ptr<mutex_t>                  mutex_p_t;
        typedef std::unique_ptr<reader_lock_t>            reader_lock_p_t;
        typedef std::unique_ptr<writer_lock_t>            writer_lock_p_t;
        typedef std::map<curl_lock_data, reader_lock_p_t> reader_map_t;
        typedef std::map<curl_lock_data, writer_lock_p_t> writer_map_t;

        std::map<curl_lock_data, mutex_p_t>    m_mutex_map;
        std::map<CURL*, reader_map_t>          m_reader_map;
        std::map<CURL*, writer_map_t>          m_writer_map;
        std::unique_ptr<CURLSH, CURLSHDeleter> m_share;
    };

    /** libcurl wrapper that performs downloads in a persistent
     * dedicated thread. Provides results on GMainLoop.
     *
     */
    class CurlEasy {
        struct CURLDeleter {
            void operator()(CURL* curl) const {
                curl_easy_cleanup( curl );
            }
        };

    public:
        typedef std::pair<std::string, DownloadResult> ResultPair;
        typedef std::function<void (std::string&&, DownloadResult&&)> EasyCallback;
        CurlEasy(const std::shared_ptr<CurlShare>& share);
        
        /** Attempts to fetch URL. download_complete is raised when
         * finished.
         *
         * Threadsafe.
         */
        void download_async(const std::string& url,
                            const EasyCallback& cb,
                            const std::size_t size_hint = 0);

    private:
        CurlEasy(const CurlEasy&)            = delete;
        CurlEasy& operator=(const CurlEasy&) = delete;

        std::unique_ptr<std::string> setup(const std::string& url);
        void download(const std::string& url,
                      const EasyCallback& cb,
                      const std::size_t size_hint);

        CallbackDispatcher                 m_dispatcher;
        std::unique_ptr<char[]>            m_error_buffer;
        std::shared_ptr<CurlShare>         m_share;
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
        typedef CurlEasy::EasyCallback CurlMultiCallback;

        /** Fetchs url and returns result in the callback.
         *
         * Threadsafe.
         */
        void download_url_async(const std::string& url,
                                const CurlMultiCallback& cb,
                                const std::size_t size_hint = 0);

    private:
        /** Serves the request queue if possible.
         *
         * m_mutex should be held before calling this function.
         */
        void start_download_from_queue();

        /** Forwards the results to cb and moves the CurlEasy back to
         * curl_queue.
         *
         * Called on GMainLoop via CurlEasy's CallbackDispatcher.
         */
        void on_download_completed(const CurlEasy* curl,
                                   const CurlMultiCallback& cb,
                                   std::string&& result,
                                   DownloadResult&& info);

        typedef std::function<void (const std::unique_ptr<CurlEasy>&)> Request;

        /** Lock access to the queues allowing download_url_async to
         * be called from any thread.
         */
        mutable std::mutex                    m_mutex;
        std::queue<std::unique_ptr<CurlEasy>> m_curl_queue;
        std::deque<std::unique_ptr<CurlEasy>> m_active_curls;
        std::queue<Request>                   m_request_queue;
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

        typedef std::function<void (const DownloadResult&)> DownloaderCallback;

        /** Downloads url and saves to filename in target_dir.
         *
         * If filename already exists, a noncolliding filename will be
         * found such as filename (2).ext.
         */
        void download_async(const std::string& url,
                            const Glib::RefPtr<Gio::File>& target_dir,
                            const std::string& filename,
                            const std::size_t size_hint,
                            const DownloaderCallback& cb);

        /** Downloads url and passes the result back to the given
         * callback on the GMainLoop.
         */
        void download_async(const std::string& url,
                            const CurlMulti::CurlMultiCallback& cb);

	private:
		Downloader& operator=(const Downloader&) = delete; // Evil func
		Downloader(const Downloader&) = delete; // Evil func

        /** Callback from CurlMulti when curl has fetched url. Called
         * on GMainLoop.
         */
        void download_complete_cb(std::string&& data,
                                  DownloadResult&& info, 
                                  const Glib::RefPtr<Gio::File>& target_dir,
                                  const std::string& filename,
                                  const DownloaderCallback& cb);

        /** Writes data to disk. Intented to be performed in a
         * dedicated thread.
         */
        void download_write(const std::string& data, DownloadResult& info,
                            const Glib::RefPtr<Gio::File>& target_dir,
                            const std::string& filename,
                            const DownloaderCallback& cb);

        CallbackDispatcher m_dispatcher;
        CurlMulti          m_curl_multi;
        Active active;
	};
}

#endif
