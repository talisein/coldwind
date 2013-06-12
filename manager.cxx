#include <iostream>
#include "manager.hxx"
#include "hasher.hxx"
#include "post.hpp"


namespace Derp {

    Manager::Manager(const std::shared_ptr<Hasher>& hasher) :
        m_hasher(hasher),
        m_downloader(std::make_shared<Downloader>()),
        m_json_parser(std::make_shared<JsonParser>(m_downloader))
    {
    }

    bool
    Manager::download_async(const Request& data)
    {
        m_hasher->hash_async(data, [this, data] {
                m_json_parser->parse_async(data, std::bind(&Manager::parse_cb,
                                                           this,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2));
            });

        return true;
    }
    
    void
    Manager::parse_cb(const ParserResult& result, const Request& request)
    {
        if (result.had_error) {
            switch (result.error_code) {
                case ParserResult::THREAD_404_ERROR:
                    signal_download_error(Error::THREAD_404);
                    break;
                case ParserResult::DOWNLOAD_ERROR:
                    signal_download_error(Error::THREAD_CURL_ERROR);
                    break;
                case ParserResult::PARSE_ERROR:
                    signal_download_error(Error::THREAD_PARSE_ERROR);
                    break;
                case ParserResult::NO_ERROR:
                    break;
            }
            return;
        }

        auto posts = std::move(result.posts);
        auto num_downloaded = std::make_shared<std::size_t>(0);
        auto num_errors     = std::make_shared<std::size_t>(0);
        auto const is_valid = [this, request](const Glib::RefPtr<Post>& post)->bool {
            return (!m_hasher->has_md5(post->get_hash_hex())
                    && post->get_width() >= request.get_min_width()
                    && post->get_height() >= request.get_min_height());
        };

        const std::size_t num_downloading = std::count_if(posts.begin(),
                                                          posts.end(),
                                                          is_valid);
        auto callback = std::bind(&Manager::download_complete_cb,
                                  this,
                                  std::placeholders::_1,
                                  request,
                                  num_downloading,
                                  num_downloaded,
                                  num_errors);
                                                  
        for (auto const & post : posts) {
            auto md5hex = post->get_hash_hex();
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

        if (num_downloading != 0)
            signal_starting_downloads(num_downloading);
        else
            signal_all_downloads_finished(0, request);
    }


    void
    Manager::download_complete_cb(const DownloadResult& info,
                                  const Request& request,
                                  const std::size_t num_downloading,
                                  const std::shared_ptr<std::size_t>& num_downloaded,
                                  const std::shared_ptr<std::size_t>& num_errors)
    {
        if (info.had_error) {
            ++(*num_errors);
            signal_download_error(Error::IMAGE_CURL_ERROR);
            std::cerr << "Error: " << info.error_str << std::endl;
        } else {
            ++(*num_downloaded);
            signal_download_finished();
            std::cout << "Info: Downloaded " << info.url << " to "
                      << info.filename << " at " << info.speed/1024.0
                      << " KiB/s" << std::endl;
        }

        if (num_downloading == (*num_downloaded + *num_errors)) {
            signal_all_downloads_finished(*num_downloaded, request);
        }
    }
}
