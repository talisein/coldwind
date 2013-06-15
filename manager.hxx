#ifndef MANAGER_HXX
#define MANAGER_HXX
#include <functional>
#include <memory>
#include <set>
#include <sigc++/signal.h>
#include <glibmm/refptr.h>
#include "downloader.hxx"
#include "request.hxx"
#include "parser.hxx"
#include "hasher.hxx"

namespace Derp {
  
	enum Error {
		THREAD_404,
		DUPLICATE_FILE,
		IMAGE_CURL_ERROR,
		THREAD_CURL_ERROR,
		THREAD_PARSE_ERROR
	};

    class Post;

    struct ManagerResult
    {
        ManagerResult();
        enum { HASHING, PARSING, DOWNLOADING, DONE, LURKING, ERROR} state;
        Request            request;
        bool               had_error;
        Error              error_code;
        std::string        error_str;
        Glib::RefPtr<Post> op; /* Thread OP post */
        std::size_t        num_downloading;
        std::size_t        num_downloaded;
        std::size_t        num_download_errors;
        DownloadResult     info; /* Valid when state is DOWNLOADING
                                  * and num_downloaded > 0 */
    };

	class Manager {
	public:
		Manager();
        ~Manager();

        typedef std::function<void (const std::shared_ptr<const ManagerResult>&)> ManagerCallback;

		bool download_async(const Request& request, const ManagerCallback& cb);

	private:
        typedef std::pair<Request, ManagerCallback> lurk_pair_t;
        struct LurkPairComparitor {
            bool operator()(const lurk_pair_t& lhs, const lurk_pair_t& rhs) const {
                return lhs.first.get_api_url() < rhs.first.get_api_url();
            };
        };

        void parse_cb(const ParserResult& parser_result,
                      const Request& request,
                      const std::shared_ptr<ManagerResult>& result,
                      const ManagerCallback& cb);

        void download_complete_cb(const DownloadResult& info,
                                  const std::shared_ptr<ManagerResult>& result,
                                  const ManagerCallback& cb);

        void request_complete(const std::shared_ptr<ManagerResult>& result,
                              const ManagerCallback& cb);

        /** Calls download_async for requests in m_lurk_list.
         *
         * Should be called from GMainLoop.
         */
        bool lurk();

        /** Called every couple seconds when there is work to queue.
         *
         * Prevents calling the 4chan JSON API too quickly.
         */
        bool lurk_cooldown(std::vector<Manager::lurk_pair_t>& list);

        static constexpr int        LURK_INTERVAL_MINUTES = 5;

        Hasher                      m_hasher;
        std::shared_ptr<Downloader> m_downloader;
        JsonParser                  m_json_parser;


        /** List of threads to lurk.
         *
         * Should only be modified on GMainLoop.
         */
        std::set<lurk_pair_t, LurkPairComparitor>  m_lurk_list;
        sigc::connection                           m_lurk_connection;
	};
}

#endif
