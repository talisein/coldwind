#include <iostream>
#include <algorithm>
#include <glibmm/convert.h>
#include <json-glib/json-glib.h>
#include "parser.hxx"
#include "downloader.hxx"
#include "request.hxx"
#include "post.hpp"

namespace Derp {

    ParserResult::ParserResult() :
        had_error(false),
        error_code(NO_ERROR)
    {
    }

    JsonParser::JsonParser(const std::shared_ptr<Downloader>& downloader) :
        m_downloader(downloader)
    {
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
                ss << "HTTP Error " << info.error_code << " \"" << info.error_str << "\"";
                ss << std::endl << "URL: " << info.url << std::endl;
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

        extern "C" {
            void _foreach_json_post(JsonArray *array,
                                    guint index_,
                                    JsonNode *element_node,
                                    gpointer user_data)
            {
                ParserResult *result = static_cast<ParserResult*>(user_data);
                GObject *cpost = json_gobject_deserialize( horizon_post_get_type(), element_node);
                if (!cpost)
                    return;
                Glib::RefPtr<Post> post = Glib::wrap(HORIZON_POST(cpost));
                if (!post)
                    return;
                if (post->has_image() && !(post->is_deleted())) {
                    result->posts.push_back(post);
                }
            }
        }
    }

    void
    JsonParser::parse(const std::string& json, const Request& request, const ParserCallback& cb)
    {
        ParserResult result;
        std::unique_ptr<::JsonParser, JsonParserDeleter> parser(json_parser_new());
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

        json_array_foreach_element(array, _foreach_json_post, &result);
        for (auto &post : result.posts) {
            post->set_board(board);
            post->set_thread_id(thread_id);
        }

        m_dispatcher(std::bind(cb, std::move(result), request));
    }
}
