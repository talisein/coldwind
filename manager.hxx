#ifndef MANAGER_HXX
#define MANAGER_HXX
#include <memory>
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
        size_t             num_downloading;
        size_t             num_downloaded;
        size_t             num_download_errors;
        DownloadResult     info; /* Valid when state is DOWNLOADING
                                  * and num_downloaded > 0 */
    };

	class Manager {
	public:
		Manager();

        typedef std::function<void (const std::shared_ptr<const ManagerResult>&)> ManagerCallback;

		bool download_async(const Request& request, const ManagerCallback& cb);

	private:
        Hasher     m_hasher;
        std::shared_ptr<Downloader> m_downloader;
        JsonParser m_json_parser;
        
        void parse_cb(const ParserResult& parser_result,
                      const Request& request,
                      const std::shared_ptr<ManagerResult>& result,
                      const ManagerCallback& cb);

        void download_complete_cb(const DownloadResult& info,
                                  const std::shared_ptr<ManagerResult>& result,
                                  const ManagerCallback& cb);
	};
}

#endif
