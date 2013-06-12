#include <iostream>
#include <algorithm>
#include <glibmm/convert.h>
#include <json-glib/json-glib.h>
#include "parser.hxx"
#include "downloader.hxx"
#include "request.hxx"
#include "post.hpp"

namespace Derp {

    JsonParser::JsonParser(const std::shared_ptr<Downloader>& downloader) :
        m_downloader(downloader)
    {
        Derp::wrap_init();
    }

    void 
    JsonParser::parse_async(const Request& request, const ParserCallback& cb)
    {
        auto const url = request.get_api_url();
        m_downloader->download_async(url, std::bind(&JsonParser::download_cb,
                                                    this,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2,
                                                    request,
                                                    cb));
    }

    void
    JsonParser::download_cb(std::string&& json, DownloadResult&& info,
                            Request& request, const ParserCallback& cb)
    {
        if (!info.had_error) {
            m_active.send(std::bind([this, request, cb](const std::string& json) {
                        parse(json, request, cb);
                    }, std::move(json)));
        } else {
            ParserResult result;
            result.had_error = true;
            if (info.error_code == 404) {
                request.mark404();
                result.error_str = "Thread 404";
                result.error_code = ParserResult::THREAD_404_ERROR;
            } else {
                std::stringstream ss;
                ss << "HTTP Error " << info.error_code;
                result.error_str = ss.str();
                result.error_code = ParserResult::DOWNLOAD_ERROR;
            }
            cb(result, request);
        }
    }

    namespace {
        struct JsonParserDeleter {
            void operator()( ::JsonParser *parser) const {
                g_object_unref(parser);
            }
        };
    }

    void
    JsonParser::parse(const std::string& json, const Request& request, const ParserCallback& cb)
    {
        ParserResult result;
		std::vector<Glib::RefPtr<Post>> posts;
        std::unique_ptr< ::JsonParser, JsonParserDeleter> parser(json_parser_new());
        auto const board = request.getBoard();
        auto const thread_id = request.get_thread_id();
		GError *merror = NULL;
		if (json_parser_load_from_data(parser.get(), json.c_str(), json.size(), &merror)) {
		} else {
            result.had_error = true;
            result.error_code = ParserResult::PARSE_ERROR;
            result.error_str = merror->message;
            g_error_free(merror);
            m_dispatcher(std::bind(cb, std::move(result), request));
            return;
		}

		JsonObject *jsonobject = json_node_get_object(json_parser_get_root(parser.get()));
		JsonArray *array = json_node_get_array(json_object_get_member(jsonobject, "posts"));
		auto const num_posts = json_array_get_length(array);
        result.posts.reserve(num_posts);

		for ( guint i = 0; i < num_posts; ++i ) {
			JsonNode *obj = json_array_get_element(array, i);
			GObject *cpost =  json_gobject_deserialize( horizon_post_get_type(), obj );
			Glib::RefPtr<Post> post = Glib::wrap(HORIZON_POST(cpost));
			post->set_board(board);
			post->set_thread_id(thread_id);
            if (post->has_image() && !(post->is_deleted()))
                result.posts.push_back(post);
		}

        m_dispatcher(std::bind(cb, std::move(result), request));
    }
}
