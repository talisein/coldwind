#include "downloader.hxx"
#include <algorithm>
#include <iostream>

namespace Derp {
    DownloadResult::DownloadResult() :
        had_error(false),
        error_code(0),
        total_time(0.),
        size(0.),
        speed(0.)
    {
    }

    namespace {
        extern "C" {
            static void
            lock_function(CURL *curl, curl_lock_data data, curl_lock_access access, void *userptr)
            {
                auto share = static_cast<CurlShare*>(userptr);
                share->lock(curl, data, access);
            }

            static void
            unlock_function(CURL *curl, curl_lock_data data, void *userptr)
            {
                auto share = static_cast<CurlShare*>(userptr);
                share->unlock(curl, data);
            }
        }
    }

    CurlShare::CurlShare() :
        m_share(curl_share_init())
    {
        auto const check_code = [](CURLSHcode code){
            if (G_UNLIKELY(code != CURLSHE_OK)) {
                std::stringstream ss;
                ss << "CurlShare Error: " << curl_share_strerror(code);
                g_error(ss.str().c_str());
            }
        };

        auto code = curl_share_setopt(m_share.get(), CURLSHOPT_LOCKFUNC, &lock_function);
        check_code(code);
        code = curl_share_setopt(m_share.get(), CURLSHOPT_UNLOCKFUNC, &unlock_function);
        check_code(code);
        code = curl_share_setopt(m_share.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
        check_code(code);
        code = curl_share_setopt(m_share.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
        check_code(code);
        code = curl_share_setopt(m_share.get(), CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
        check_code(code);
        code = curl_share_setopt(m_share.get(), CURLSHOPT_USERDATA, this);
        check_code(code);
    }

    CurlShare::mutex_t& CurlShare::get_mutex(curl_lock_data data)
    {
        std::lock_guard<mutex_t> lock(m_mutex);
        auto iter = m_mutex_map.find(data);
        if (iter == std::end(m_mutex_map)) {
            std::tie(iter, std::ignore) = m_mutex_map.emplace(std::piecewise_construct,
                                                              std::forward_as_tuple(data),
                                                              std::forward_as_tuple());
        }
        return iter->second;
    }

    void
    CurlShare::lock(CURL* curl, curl_lock_data data, curl_lock_access access)
    {
        auto &mutex = get_mutex(data);

        if (access == CURL_LOCK_ACCESS_SINGLE) {
            mutex.lock();
        } else if (G_LIKELY(access == CURL_LOCK_ACCESS_SHARED)) {
            mutex.lock();
        } else {
            g_error("CurlShare Error: Unexpected lock access type");
        }
    }

    void
    CurlShare::unlock(CURL* curl, curl_lock_data data)
    {
        auto &mutex = get_mutex(data);
        mutex.unlock();
    }

    CurlEasy::CurlEasy(const std::shared_ptr<CurlShare>& share) :
        m_error_buffer(std::make_unique<char[]>(CURL_ERROR_SIZE)),
        m_share(share),
        m_curl(curl_easy_init())
    {
    }

    namespace {
        extern "C" {
            size_t curleasy_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
            {
                std::string* str_p = static_cast<std::string*>(userdata);
                str_p->append(static_cast<char*>(ptr), size*nmemb);
                return size*nmemb;
            }
        }
    }

    std::unique_ptr<std::string> CurlEasy::setup(const std::string& url)
    {
        bool setup_ok = true;
        auto check_code = [&setup_ok](CURLcode code) {
            if (code != CURLE_OK) {
                setup_ok = false;
            }
        };

        std::unique_ptr<std::string> buffer = std::make_unique<std::string>();

        auto code = curl_easy_setopt(m_curl.get(), CURLOPT_URL, url.c_str());
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_WRITEFUNCTION, &curleasy_write_cb);
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_WRITEDATA, buffer.get());
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_FAILONERROR, 1);
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_TIMEOUT, 60);
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_NOSIGNAL, 1);
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_ERRORBUFFER, m_error_buffer.get());
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_SHARE, m_share->m_share.get());
        check_code(code);
        code = curl_easy_setopt(m_curl.get(), CURLOPT_USERAGENT, "coldwind/1.0 (linux)");
        check_code(code);

        if (setup_ok) {
            return buffer;
        } else {
            return nullptr;
        }
    }

    void CurlEasy::download_async(const std::string& url,
                                  const EasyCallback& cb,
                                  const std::size_t size_hint)
    {
        active.send([this, url, cb, size_hint]{download(url, cb, size_hint);});
    }

    void CurlEasy::download(const std::string& url,
                            const EasyCallback& cb,
                            const std::size_t size_hint)
    {
        DownloadResult info;
        info.url = url;
        auto buffer = setup(url);
        if (buffer) {
            buffer->reserve(size_hint);
            auto code = curl_easy_perform(m_curl.get());
            curl_easy_getinfo(m_curl.get(), CURLINFO_SIZE_DOWNLOAD,  &info.size);
            if (code == CURLE_OK && 0 != info.size) {
                curl_easy_getinfo(m_curl.get(), CURLINFO_TOTAL_TIME,     &info.total_time);
                curl_easy_getinfo(m_curl.get(), CURLINFO_SPEED_DOWNLOAD, &info.speed);
            } else {
                std::stringstream ss;
                curl_easy_getinfo(m_curl.get(), CURLINFO_RESPONSE_CODE,  &info.error_code);
                ss << "Failed downloading " << url << " (" << info.error_code << "): " << static_cast<char*>(m_error_buffer.get());
                info.error_str = ss.str();
                info.had_error = true;
            }
        } else {
            std::stringstream ss;
            ss << "Unable to setup download for " << url << ". Download aborted.";
            info.error_str = ss.str();
            info.error_code = -1;
            info.had_error = true;
        }

        cb(std::move(*buffer), std::move(info));
    }

    CurlMulti::CurlMulti(const int max_connections)
    {
        auto share = std::make_shared<CurlShare>();
        for(int i = 0; i < max_connections; ++i) {
            m_curl_stack.emplace(std::make_unique<CurlEasy>(share));
        }
    }

    void CurlMulti::start_download_from_queue()
    {
        while (!m_curl_stack.empty()) {
            if (m_request_queue.empty()) {
                break;
            } else {
                auto fn = m_request_queue.front(); m_request_queue.pop();
                auto curl = std::move(m_curl_stack.top()); m_curl_stack.pop();
                fn(curl);
                m_active_curls.push_back(std::move(curl));
            }
        }
    }

    void CurlMulti::on_download_completed(const CurlEasy* curl,
                                          const CurlMultiCallback& cb,
                                          std::string&& result,
                                          DownloadResult&& info)
    {
        cb(std::move(result), std::move(info));

        std::lock_guard<std::mutex> lock(m_mutex);
        /* Move the curl handle back to m_curl_stack */
        auto iter = std::find_if(m_active_curls.begin(),
                                 m_active_curls.end(),
                                 [&curl](const std::unique_ptr<CurlEasy>& easy){
                                     return easy.get() == curl;
                                 });
        if (G_LIKELY( iter != m_active_curls.end() )) {
            m_curl_stack.push(std::move(*iter));
            m_active_curls.erase(iter);
            /* Check for pending requests */
            if (!m_request_queue.empty())
                start_download_from_queue();
        }
    }

    void CurlMulti::download_url_async(const std::string& url,
                                       const CurlMultiCallback& cb,
                                       const std::size_t size_hint)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_request_queue.push([this, url, cb, size_hint](const std::unique_ptr<CurlEasy>& curl) {
                curl->download_async(url,
                                     std::bind(&CurlMulti::on_download_completed,
                                               this,
                                               curl.get(),
                                               cb,
                                               std::placeholders::_1,
                                               std::placeholders::_2),
                                     size_hint);
            });
        if (!m_curl_stack.empty())
            start_download_from_queue();
    }

    Downloader::Downloader() :
        m_curl_multi(COLDWIND_CURL_CONNECTIONS)
    {
    }

    void Downloader::download_async(const std::string& url,
                                    const Glib::RefPtr<Gio::File>& target_dir,
                                    const std::string& filename,
                                    const std::size_t size_hint,
                                    const DownloaderCallback& cb)

    {
        m_curl_multi.download_url_async(url,
                                        std::bind(&Downloader::download_complete_cb,
                                                  this,
                                                  std::placeholders::_1, /* data */
                                                  std::placeholders::_2, /* download info */
                                                  target_dir,
                                                  filename,
                                                  cb),
                                        size_hint);
    }

    void Downloader::download_async(const std::string& url, const CurlMulti::CurlMultiCallback& cb)
    {
        m_curl_multi.download_url_async(url, cb);
    }

    namespace {
        static bool
        ensure_target_directory_exists(const Glib::RefPtr<Gio::File>& dir,
                                       DownloadResult& info)
        {
            if (!dir->query_exists()) {
                try {
                    dir->make_directory_with_parents();
                    return true;
                } catch (Gio::Error& e) {
                    std::stringstream ss;
                    ss << "Unable to create directory " << dir->get_path() << ": "
                       << e.what();
                    info.had_error = true;
                    info.error_str = ss.str();
                    info.error_code = -2;
                    return false;
                }
            } else {
                return true;
            }
        }
    }

    void Downloader::download_write(const std::string& data, DownloadResult& info,
                                    const Glib::RefPtr<Gio::File>& target_dir,
                                    const std::string& filename,
                                    const DownloaderCallback& cb)
    {
        /* Ensure target directory exists */
        if (!ensure_target_directory_exists(target_dir, info)) {
            m_dispatcher(std::bind(cb, info));
            return;
        }

        /* Open new, non-existing file */
        auto const last_period = filename.find_last_of(".");
        auto const ext = filename.substr(last_period);
        auto const name = filename.substr(0, last_period);
        auto file = target_dir->get_child_for_display_name(filename);
        Glib::RefPtr<Gio::FileOutputStream> fos;

        for (unsigned int i = 1; !fos; ++i) {
            try {
                fos = file->create_file(Gio::FILE_CREATE_PRIVATE);
            } catch (Gio::Error& e) {
                auto const code = e.code();
                if (code == Gio::Error::EXISTS || code == Gio::Error::IS_DIRECTORY) {
                    std::stringstream ss;
                    ss << name << " (" << i << ")" << ext;
                    file = target_dir->get_child_for_display_name(ss.str());
                } else {
                    std::stringstream ss;
                    ss << "Saving " << file->get_path() << " failed: " << e.what();
                    info.had_error = true;
                    info.error_code = -2;
                    info.error_str = ss.str();
                    m_dispatcher(std::bind(cb, info));
                    return;
                }
            }
        };

        /* Write data to file */
        try {
            gsize written;
            fos->write_all(data, written);
            fos->close();
            info.filename = file->get_path();
            info.file = file;
            m_dispatcher(std::bind(cb, info));
        } catch (Gio::Error& e) {
            std::stringstream ss;
            ss << "Saving " << file->get_path() << " failed: " << e.what();
            try {
                file->remove();
            } catch (Gio::Error& e) {
                ss << std::endl << "Also unable to cleanup corrupt file "
                   << file->get_path() << ": " << e.what();
            }
            info.had_error = true;
            info.error_str = ss.str();
            info.error_code = -2;
            m_dispatcher(std::bind(cb, info));
        }
    }

    void Downloader::download_complete_cb(std::string&& data,
                                          DownloadResult&& info,
                                          const Glib::RefPtr<Gio::File>& target_dir,
                                          const std::string& filename,
                                          const DownloaderCallback& cb)
    {
        if (!info.had_error) {
            active.send(std::bind(&Downloader::download_write,
                                  this,
                                  std::move(data),
                                  std::move(info),
                                  target_dir,
                                  filename,
                                  cb));
        } else {
            cb(info);
        }
    }
}
