#include <iostream>
#include "manager.hxx"
#include "post.hpp"
#include <glibmm/main.h>

namespace sigc {
    SIGC_FUNCTORS_DEDUCE_RESULT_TYPE_WITH_DECLTYPE
}

namespace Derp {

    ManagerResult::ManagerResult() :
        state(HASHING),
        had_error(false),
        error_code(Error::DUPLICATE_FILE),
        num_downloading(0),
        num_downloaded(0),
        num_download_errors(0)
    {
    }

    Manager::Manager() :
        m_downloader(std::make_shared<Downloader>()),
        m_json_parser(m_downloader)
    {
        m_lurk_connection = Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this,
                                                                                 &Manager::lurk),
                                                                   LURK_INTERVAL_MINUTES * 60,
                                                                   Glib::PRIORITY_LOW);
    }

    Manager::~Manager()
    {
        m_lurk_connection.disconnect();
    }

    bool
    Manager::download_async(const Request& request, const ManagerCallback& cb)
    {
        auto result = std::make_shared<ManagerResult>();
        if (!request.getHashDirectory()) {
            result->had_error = true;
            result->state = ManagerResult::ERROR;
            result->request = request;
            result->error_str = "Target directory is null";
            cb(result);
            return true;
        }

        result->had_error = false;
        result->state = ManagerResult::HASHING;
        result->request = request;
        cb(result);
        m_hasher.hash_async(request, [this, result, cb] {
                result->state = ManagerResult::PARSING;
                cb(result);
                m_json_parser.parse_async(result->request,
                                           std::bind(&Manager::parse_cb,
                                                     this,
                                                     std::placeholders::_1,
                                                     std::placeholders::_2,
                                                     result,
                                                     cb));
            });

        return true;
    }

    void
    Manager::parse_cb(const ParserResult& parser_result,
                      const Request& request,
                      const std::shared_ptr<ManagerResult>& result,
                      const ManagerCallback& cb)
    {
        result->request = request; // Parser may modify request
        if (parser_result.had_error) {
            result->had_error = true;
            result->state = ManagerResult::ERROR;
            result->error_str = parser_result.error_str;
            switch (parser_result.error_code) {
                case ParserResult::THREAD_404_ERROR:
                    result->error_code = Error::THREAD_404;
                    result->request.mark404();
                    break;
                case ParserResult::DOWNLOAD_ERROR:
                    result->error_code = Error::THREAD_CURL_ERROR;
                    break;
                case ParserResult::PARSE_ERROR:
                    result->error_code = Error::THREAD_PARSE_ERROR;
                    break;
                case ParserResult::NO_ERROR:
                    break;
            }
            cb(result);
            request_complete(result, cb);
            return;
        }

        auto posts = std::move(parser_result.posts);
        auto const is_valid = [this, request](const Glib::RefPtr<Post>& post)->bool {
            return (!m_hasher.has_md5(post->get_hash())
                    && post->get_width() >= request.get_min_width()
                    && post->get_height() >= request.get_min_height());
        };

        result->op = posts.front();
        result->num_downloaded = 0;
        result->num_download_errors = 0;
        result->num_downloading = std::count_if(posts.begin(),
                                                posts.end(),
                                                is_valid);

        result->state = ManagerResult::DOWNLOADING;
        cb(result);

        if (result->num_downloading == 0) {
            if (result->op->get_imagelimit()) {
                result->request.mark404();
            }
            request_complete(result, cb);
        }

        auto callback = std::bind(&Manager::download_complete_cb,
                                  this,
                                  std::placeholders::_1,
                                  result,
                                  cb);
        for (auto const & post : posts) {
            if (is_valid(post)) {
                std::string filename;
                if (request.useOriginalFilename()) {
                    filename = post->get_original_filename() + post->get_image_ext();
                } else {
                    filename = post->get_renamed_filename() + post->get_image_ext();
                }
                m_downloader->download_async(post->get_image_url(),
                                             request.getDirectory(),
                                             filename,
                                             post->get_fsize(),
                                             callback);
            }
        }
    }

    void
    Manager::download_complete_cb(const DownloadResult& info,
                                  const std::shared_ptr<ManagerResult>& result,
                                  const ManagerCallback& cb)
    {
        if (info.had_error) {
            result->had_error = true;
            result->error_code = Error::IMAGE_CURL_ERROR;
            result->error_str = info.error_str;
            ++(result->num_download_errors);
        } else {
            result->had_error = false;
            ++(result->num_downloaded);
            m_hasher.hash_one_async(info.file);
        }
        result->info = info;
        cb(result);

        if (result->num_downloading == (result->num_downloaded + result->num_download_errors)) {
            request_complete(result, cb);
        }
    }

    void
    Manager::request_complete(const std::shared_ptr<ManagerResult>& result,
                              const ManagerCallback& cb)
    {
        result->had_error = false;

        auto pair = std::make_pair(result->request, cb);
        auto iter = m_lurk_list.find(pair);
        if (iter != m_lurk_list.end()) {
            if (iter->first.get_request_id() != result->request.get_request_id()) {
                auto res = std::make_shared<ManagerResult>();
                res->state = ManagerResult::DONE;
                res->request = iter->first;
                iter->second(res);
            }
            m_lurk_list.erase(iter);
        }

        if (result->request.isExpired()) {
            result->state = ManagerResult::DONE;
        } else {
            result->state = ManagerResult::LURKING;
            m_lurk_list.insert(std::move(pair));
        }

        cb(result);
    }

    bool
    Manager::lurk()
    {
        std::vector<lurk_pair_t> worklist;
        worklist.reserve(m_lurk_list.size());
        for ( auto & pair : m_lurk_list ) {
            auto request = pair.first;
            request.decrementMinutes(LURK_INTERVAL_MINUTES);
            worklist.push_back(std::make_pair(request, pair.second));
        }

        if (worklist.size() > 0) {
            auto fn = sigc::bind(sigc::mem_fun(*this, &Manager::lurk_cooldown), worklist);
            Glib::signal_timeout().connect_seconds(fn, 5, Glib::PRIORITY_LOW);
        }

        return G_SOURCE_CONTINUE;
    }

    bool
    Manager::lurk_cooldown(std::vector<Manager::lurk_pair_t>& list)
    {
        if (list.size() > 0) {
            download_async(list.back().first, list.back().second);
            list.pop_back();
        }

        if (list.size() > 0)
            return G_SOURCE_CONTINUE;
        else
            return G_SOURCE_REMOVE;
    }
}
