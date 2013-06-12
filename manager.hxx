#ifndef MANAGER_HXX
#define MANAGER_HXX
#include <memory>
#include <sigc++/signal.h>
#include <glibmm/refptr.h>
#include "request.hxx"
#include "parser.hxx"
#include "downloader.hxx"

namespace Derp {
  
	enum Error {
		THREAD_404,
		DUPLICATE_FILE,
		IMAGE_CURL_ERROR,
		THREAD_CURL_ERROR,
		THREAD_PARSE_ERROR
	};

    class Hasher;
    class Post;

    struct ManagerResult
    {
        enum { HASHING, PARSING, DOWNLOADING, DONE, LURKING } state;
        Request            request;
        bool               had_error;
        Error              error_code;
        std::string        error_str;
        Glib::RefPtr<Post> op; /* Thread OP post */
        size_t             num_downloading;
        size_t             num_downloaded;
        size_t             num_download_errors;
        DownloadResult     info; /* Valid when state is DOWNLOADING
                                  * and num_downloaded > 0 */
    };

	class Manager {
	public:
		Manager(const std::shared_ptr<Hasher>& hasher);

		bool download_async(const Request& data);
		sigc::signal<void, int, const Request&> signal_all_downloads_finished;
		sigc::signal<void> signal_download_finished;
		sigc::signal<void, int> signal_starting_downloads;
		sigc::signal<void, const Error&> signal_download_error;

	private:
        std::shared_ptr<Hasher> m_hasher;
        std::shared_ptr<Downloader> m_downloader;
        std::shared_ptr<JsonParser> m_json_parser;
        
        void parse_cb(const ParserResult& result, const Request& request);

        void download_complete_cb(const DownloadResult& info,
                                  const Request& request,
                                  const std::size_t num_downloading,
                                  const std::shared_ptr<std::size_t>& num_downloaded,
                                  const std::shared_ptr<std::size_t>& num_errors);
	};
}

#endif
