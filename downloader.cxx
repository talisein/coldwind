#include "downloader.hxx"
#include <algorithm>
#include <iostream>

namespace Derp {
    namespace {
        extern "C" {
            static void 
            lock_function(CURL *curl, curl_lock_data data, curl_lock_access access, void *userptr)
            {
                //auto mutex_map_p = static_cast<std::map<curl_lock_data, std::unique_ptr<std::mutex>>*>(userptr);
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

    void CurlShare::lock(CURL* curl, curl_lock_data data, curl_lock_access access)
    {
        auto iter = m_mutex_map.find(data);
        if (iter == m_mutex_map.end()) {
            auto result = m_mutex_map.insert(std::make_pair(data, std::unique_ptr<Glib::Threads::RWLock>(new Glib::Threads::RWLock())));
            iter = result.first;
        }

        if (G_LIKELY(iter != m_mutex_map.end())) {
            if (access == CURL_LOCK_ACCESS_SINGLE) {
                auto map_iter = m_writer_map.find(curl);
                if (map_iter == m_writer_map.end()) {
                    auto result = m_writer_map.insert(std::make_pair(curl, std::map<curl_lock_data, std::unique_ptr<Glib::Threads::RWLock::WriterLock>>()));
                    map_iter = result.first;
                }
                
                if (G_LIKELY( map_iter != m_writer_map.end() ))
                    map_iter->second.insert(std::make_pair(data, std::unique_ptr<Glib::Threads::RWLock::WriterLock>(new Glib::Threads::RWLock::WriterLock(*(iter->second)))));
                else
                    g_error("CurlShare Error: Unable to create writer lock map!");

            } else if (access == CURL_LOCK_ACCESS_SHARED) {
                auto map_iter = m_reader_map.find(curl);
                if (map_iter == m_reader_map.end()) {
                    auto result = m_reader_map.insert(std::make_pair(curl, std::map<curl_lock_data, std::unique_ptr<Glib::Threads::RWLock::ReaderLock>>()));
                    map_iter = result.first;
                }

                if (G_LIKELY( map_iter != m_reader_map.end() ))
                    map_iter->second.insert(std::make_pair(data, std::unique_ptr<Glib::Threads::RWLock::ReaderLock>(new Glib::Threads::RWLock::ReaderLock(*(iter->second)))));
                else
                    g_error("CurlShare Error: Unable to create reader lock map!");
            }
        } else {
            g_error("CurlShare Error: Unable to create lock!");
        }
    }

    void CurlShare::unlock(CURL* curl, curl_lock_data data)
    {
        auto write_iter = m_writer_map.find(curl);
        if (write_iter != m_writer_map.end()) {
            auto lock_iter = write_iter->second.find(data);
            if (lock_iter != write_iter->second.end())
                write_iter->second.erase(lock_iter);
        }

        auto read_iter = m_reader_map.find(curl);
        if (read_iter != m_reader_map.end()) {
            auto lock_iter = read_iter->second.find(data);
            if (lock_iter != read_iter->second.end())
                read_iter->second.erase(lock_iter);
        }
    }

    CurlEasy::CurlEasy(const std::shared_ptr<CurlShare>& share) :
        m_error_buffer(new char[CURL_ERROR_SIZE]),
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
    
    bool CurlEasy::setup(const std::string& url)
    {
        bool setup_ok = true;
        auto check_code = [this, &setup_ok, &url](CURLcode code) {
            if (code != CURLE_OK) {
                setup_ok = false;
                std::stringstream ss;
                ss << "Failed to set CURL option for " << url << ": " << curl_easy_strerror(code);
                signal_error(ss.str());
            }
        };

        std::unique_ptr<std::string> buffer(new std::string());
        buffer->reserve(50*1024);

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

        if (setup_ok) {
            m_buffer = std::move(buffer);
        }
        return setup_ok;
    }

    void CurlEasy::download_async(const std::string& url)
    {
        active.send([this, url]{download(url);});
    }

    void CurlEasy::download(const std::string& url)
    {
        DownloadInfo info;
        info.url = url;
        info.had_error = false;
        if (setup(url)) {
            auto code = curl_easy_perform(m_curl.get());
            if (code == CURLE_OK) {
                curl_easy_getinfo(m_curl.get(), CURLINFO_TOTAL_TIME, &info.total_time);
                curl_easy_getinfo(m_curl.get(), CURLINFO_SIZE_DOWNLOAD, &info.size);
                curl_easy_getinfo(m_curl.get(), CURLINFO_SPEED_DOWNLOAD, &info.speed);
            } else {
                std::stringstream ss;
                ss << "Failed downloading " << url << ": " << static_cast<char*>(m_error_buffer.get());
                signal_error(ss.str());
                info.had_error = true;
            }
        } else {
            signal_error(std::string("Unable to setup download for ") + url + ". Download aborted.");
            info.had_error = true;
        }

        download_complete(std::make_pair(std::move(*m_buffer), std::move(info)));
    }

    CurlMulti::CurlMulti(const int max_connections)
    {
        auto share = std::make_shared<CurlShare>();
        for(int i = 0; i < max_connections; ++i) {
            curl_queue.emplace(new CurlEasy(share));
            curl_queue.back()->signal_error.connect(sigc::bind(sigc::mem_fun(*this, &CurlMulti::on_download_error),
                                                                 sigc::ref(*(curl_queue.back()))));
            curl_queue.back()->download_complete.connect(sigc::bind(sigc::mem_fun(*this, &CurlMulti::on_download_completed),
                                                                    sigc::ref(*(curl_queue.back()))));
        }
    }

    void CurlMulti::on_download_error(CurlEasy& curl)
    {
        while (!curl.signal_error.empty()) {
            signal_error(std::move(curl.signal_error.front()));
            curl.signal_error.pop();
        }
    }

    void CurlMulti::start_download_from_queue()
    {
        while (!curl_queue.empty()) {
            if (request_queue.empty()) {
                break;
            } else {
                auto url = request_queue.front(); request_queue.pop();
                auto curl = std::move(curl_queue.front()); curl_queue.pop();
                curl->download_async(url);
                active_curls.push_back(std::move(curl));
            }
        }
    }

    void CurlMulti::on_download_completed(CurlEasy& curl)
    {
        while (!curl.download_complete.empty()) {
            auto pair = std::move(curl.download_complete.front());
            curl.download_complete.pop();
            /* Find and call callback, then remove */
            auto iter = callback_map.find(pair.second.url);
            if (iter != callback_map.end()) {
                iter->second(std::move(pair.first), std::move(pair.second));
                callback_map.erase(iter);
            } else {
                std::stringstream ss;
                ss << "Downloaded " << pair.second.url << " but there is no callback to serve.";
                signal_error(ss.str()); 
            }
        }

        /* Move the curl handle back to curl_queue */
        auto iter = std::find_if(active_curls.begin(),
                                 active_curls.end(),
                                 [&curl](const std::unique_ptr<CurlEasy>& easy){
                                     return easy.get() == &curl;
                                 });
        if (G_LIKELY( iter != active_curls.end() )) {
            curl_queue.push(std::move(*iter));
            active_curls.erase(iter);
            /* Check for pending requests */
            start_download_from_queue();
        } else {
            signal_error("Download completed but can't find curl handle to return to queue");
        }
    }

    void CurlMulti::download_url_async(const std::string& url, const CurlMultiCallback& cb)
    {
        callback_map.insert(std::make_pair(url, cb));
        request_queue.push(url);
        start_download_from_queue();
    }
                      
    Downloader::Downloader() :
        m_curl_multi(COLDWIND_CURL_CONNECTIONS)
    {
        m_curl_multi.signal_error.connect([this](const std::string& msg) {
                signal_error(msg);
            });

        signal_internal_error.connect([this]{
                while(!signal_internal_error.empty()) {
                    signal_error(std::move(signal_internal_error.front()));
                    signal_internal_error.pop();
                }
            });

        signal_internal_download_complete.connect([this]{
                while (!signal_internal_download_complete.empty()) {
                    signal_download_complete(std::move(signal_internal_download_complete.front()));
                    signal_internal_download_complete.pop();
                }
            });
    }
    
    void Downloader::download_async(const std::string& url,
                                    const Glib::RefPtr<Gio::File>& target_dir,
                                    const std::string& filename)
    {
        m_curl_multi.download_url_async(url, std::bind(&Downloader::download_complete_cb,
                                                       this,
                                                       std::placeholders::_1,
                                                       std::placeholders::_2,
                                                       target_dir,
                                                       filename));
    }

    void Downloader::download_async(const std::string& url, const CurlMulti::CurlMultiCallback& cb)
    {
        m_curl_multi.download_url_async(url, cb);
    }
                        
    void Downloader::download_async(const std::list<Image>& imgs,
                                    const Request& request) {
        for (auto & img : imgs) {
            download_async(static_cast<std::string>(img.getUrl()), request.getDirectory(), img.getFilename());
        }
    }

    namespace {
        static bool
        ensure_target_directory_exists(const Glib::RefPtr<Gio::File>& dir,
                                       MessageDispatcher<std::string>& signal_error)
        {
            if (!dir->query_exists()) {
                try {
                    dir->make_directory_with_parents();
                    return true;
                } catch (Gio::Error e) {
                    std::stringstream ss;
                    ss << "Unable to create directory " << dir->get_path() << ": "
                       << e.what();
                    signal_error(ss.str());
                    return false;
                }
            } else {
                return true;
            }
        }
    }

    void Downloader::download_write(const std::string& data, DownloadInfo& info,
                                    const Glib::RefPtr<Gio::File>& target_dir,
                                    const std::string& filename)
    {
        /* Ensure target directory exists */
        if (!ensure_target_directory_exists(target_dir, signal_internal_error)) {
            signal_download_error();
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
            } catch (Gio::Error e) {
                auto const code = e.code();
                if (code == Gio::Error::EXISTS || code == Gio::Error::IS_DIRECTORY) {
                    std::stringstream ss;
                    ss << name << " (" << i << ")" << ext;
                    file = target_dir->get_child_for_display_name(ss.str());
                } else {
                    std::stringstream ss;
                    ss << "Saving " << file->get_path() << " failed: " << e.what();
                    signal_internal_error(ss.str());
                    signal_download_error();
                    break;
                }
            }
        };
                        
        /* Write data to file */
        try {
            gsize written;
            fos->write_all(data, written);
            fos->close();
            info.filename = file->get_basename();
            signal_download_finished();
            signal_internal_download_complete(info);
        } catch (Gio::Error e) {
            std::stringstream ss;
            ss << "Saving " << file->get_path() << " failed: " << e.what();
            signal_internal_error(ss.str());
            try {
                file->remove();
            } catch (Gio::Error e) {
                std::stringstream ss;
                ss << "Unable to cleanup corrupt file " << file->get_path() << ": " << e.what();
                signal_internal_error(ss.str());
            }
            signal_download_error();
        }        
    }

    void Downloader::download_complete_cb(std::string&& data,
                                          DownloadInfo&& info,
                                          const Glib::RefPtr<Gio::File>& target_dir,
                                          const std::string& filename)
    {
        if (!info.had_error) {
            active.send(std::bind(&Downloader::download_write, this,
                                  std::move(data), std::move(info),
                                  target_dir, filename));
        } else {
            signal_download_error();
        }
    }
}
