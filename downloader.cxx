#include "downloader.hxx"
#include <curl/curl.h>
#include <iostream>
#include <glibmm/thread.h>

Derp::Downloader::Downloader() : m_threadPool(4) 
{
}

void Derp::Downloader::download_async(const std::list<Derp::Image>& imgs, const std::string& path) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::download_imgs), imgs, path), false);
}

void Derp::Downloader::download_imgs(std::list<Derp::Image> imgs, const std::string& path) {
  while ( !imgs.empty() ) {
    m_threadPool.push(sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::download_url), imgs.front().getUrl(), path ));
    imgs.pop_front();
  }
}

void Derp::Downloader::download_url(const Glib::ustring& url, std::string path) {
  CURL *curl = curl_easy_init();
  CURLcode res;
  Glib::ustring filename(url);
  filename.erase(0, url.find_last_of("/"));
  FILE* file = fopen(path.append(filename).c_str(), "w+");

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    res = curl_easy_perform(curl);
    if (res != 0) {
      std::cerr << "An error occured with curl\n";
    }
    curl_easy_cleanup(curl);
  }
  fclose(file);

  signal_download_finished();
}
