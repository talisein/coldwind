#include "manager.hxx"

Derp::Manager::Manager() : is_working(false)
{
  m_parser.signal_parsing_finished.connect(sigc::mem_fun(*this, &Derp::Manager::parsing_finished));
  m_hasher.signal_hashing_finished.connect(sigc::mem_fun(*this, &Derp::Manager::hashing_finished));
  m_downloader.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Manager::download_finished));
}

bool Derp::Manager::download_async(const Derp::Lurk_Data& data) {
  if (!is_working) {
    is_working = true;
    m_data = data;
    is_hashing = is_parsing = true;
    m_parser.parse_async(m_data.thread_url);
    m_hasher.hash_async(m_data.target_directory);
    return true;
  } else {
    return false; 
 }
}

void Derp::Manager::parsing_finished() {
  is_parsing = false;
  try_download();
}

void Derp::Manager::hashing_finished() {
  is_hashing = false;
  try_download();
}

void Derp::Manager::try_download() {
  if ( !(is_parsing || is_hashing) ) {
    num_downloaded = 0;
    num_downloading = m_parser.request_downloads(m_downloader, &m_hasher, m_data.target_directory->get_path(), m_data.xDim, m_data.yDim);
    if ( num_downloading == 0 ) {
      done();
    } else {
      signal_starting_downloads(num_downloading);
    }
  }
}

void Derp::Manager::download_finished() {
  num_downloaded++;
  signal_download_finished();
  if (num_downloading == num_downloaded) {
    done();
  }
}

void Derp::Manager::done() {
  signal_all_downloads_finished(num_downloaded, m_data);
  is_working = false;
}
