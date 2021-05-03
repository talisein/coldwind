#ifndef PARSER_HXX
#define PARSER_HXX

#include <list>
#include "callback_dispatcher.hpp"
#include "active.hpp"
#include "post.hpp"

namespace Derp {

    class Downloader;
    class Request;
    struct DownloadResult;
    struct ParserResult;

    class JsonParser
    {
    public:
        JsonParser(const std::shared_ptr<Downloader>&);

        typedef std::list<Glib::RefPtr<Derp::Post>> ImageList_t;

        typedef std::function<void (const ParserResult&, const Request&)> ParserCallback;
        /** Return in cb a list of 4chan posts in the thread specified
         * by Request that have images.
         */
        void parse_async(const Request& request, const ParserCallback& cb);

    private:
        /* Forward the downloaded JSON to parse(), or callback on
         * error.
         */
        void download_cb(std::string&& json, DownloadResult&& info,
                         Request& request, const ParserCallback& cb);

        void parse(const std::string& json, const Request& request, const ParserCallback& cb);

        std::shared_ptr<Downloader> m_downloader;
        CallbackDispatcher m_dispatcher;
        Active m_active;
    };

    struct ParserResult
    {
        ParserResult();
        bool had_error;
        enum { NO_ERROR, THREAD_404_ERROR, DOWNLOAD_ERROR, PARSE_ERROR } error_code;
        std::string error_str;
        JsonParser::ImageList_t posts;
    };


}

#endif
