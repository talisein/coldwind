#include <iostream>
#include "manager.hxx"
#include "post.hpp"

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
    }

    bool
    Manager::download_async(const Request& request, const ManagerCallback& cb)
    {
        auto result = std::make_shared<ManagerResult>();
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
            return;
        }

        auto posts = std::move(parser_result.posts);
        auto const is_valid = [this, request](const Glib::RefPtr<Post>& post)->bool {
            return (!m_hasher.has_md5(post->get_hash_hex())
                    && post->get_width() >= request.get_min_width()
                    && post->get_height() >= request.get_min_height());
        };

        result->op = posts.front();
        result->num_downloaded = 0;
        result->num_download_errors = 0;
        result->num_downloading = std::count_if(posts.begin(),
                                                posts.end(),
                                                is_valid);
        cb(result);
        auto callback = std::bind(&Manager::download_complete_cb,
                                  this,
                                  std::placeholders::_1,
                                  result,
                                  cb);
        if (result->num_downloading == 0) {
            result->state = ManagerResult::DONE;
            cb(result);
        } else {
            result->state = ManagerResult::DOWNLOADING;
            cb(result);
        }
                                                  
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
        }
        result->info = info;
        cb(result);

        if (result->num_downloading == (result->num_downloaded + result->num_download_errors)) {
            result->state = ManagerResult::DONE;
            cb(result);
        }
    }
}
