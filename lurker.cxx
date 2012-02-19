#include "lurker.hxx"
#include <iostream>
#include <glibmm/thread.h>

Derp::Lurker::Lurker() : is_hashing(false), is_parsing(false), num_downloaded(0)
{
  m_parser.signal_parsing_finished.connect(sigc::mem_fun(*this, &Derp::Lurker::parsing_finished));
  m_hasher.signal_hashing_finished.connect(sigc::mem_fun(*this, &Derp::Lurker::hashing_finished));
  m_downloader.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Lurker::download_finished));
}

/*
  Since the list is accessed in a worker thread, we need to take a
  mutex before we access it. But we don't want the GUI thread to
  freeze waiting for run() to finish. So we have to make an async
  method.
 */
void Derp::Lurker::add_async(const Derp::Lurk_Data& data) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Derp::Lurker::add), data), false);
}

void Derp::Lurker::add(const Derp::Lurk_Data& data) {
  Glib::Mutex::Lock lock(m_list_lock);
  m_list.push_back(data);
}

void Derp::Lurker::run() {
  if (!m_list_lock.trylock()) {
    std::cerr << "Lurker got called to run(), but is still working on a previous run. Skipping." << std::endl;
    return;
  }
  std::cout << "The lurker is running on the main thread." << std::endl;  

  total_downloaded = 0;
  iter = m_list.begin();
  if (iter != m_list.end()) {
    iteration_next();
  } else {
    m_list_lock.unlock();
  }
}

void Derp::Lurker::iteration_next() {
  is_hashing = is_parsing = true;
  m_parser.parse_async(iter->thread_url);
  m_hasher.hash_async(iter->target_directory);
}

void Derp::Lurker::iteration_finish() {
  iter->minutes -= 1;
  total_downloaded += num_downloaded;
  iter++;
  if (iter == m_list.end()) {
    std::cout << "Lurker downloaded " << total_downloaded << " images total\n";
    std::cout << "There were " << m_list.size() << " threads being monitored, now only ";
    m_list.remove_if([](Lurk_Data data) { return data.minutes <= 0; });
    std::cout << m_list.size() << " remain." << std::endl;

    m_list_lock.unlock();
  } else {
    iteration_next();
  }
}

void Derp::Lurker::parsing_finished() {
  is_parsing = false;
  try_download();
}

void Derp::Lurker::hashing_finished() {
  is_hashing = false;
  try_download();
}

void Derp::Lurker::try_download() {
  if ( !(is_parsing || is_hashing) ) {
    num_downloaded = 0;
    num_downloading = m_parser.request_downloads(m_downloader, &m_hasher, iter->target_directory->get_path(), iter->xDim, iter->yDim);
    if ( num_downloading == 0 ) {
      iteration_finish();
    } 
  }
}

void Derp::Lurker::download_finished() {
  num_downloaded++;
  if (num_downloading == num_downloaded) {
    iteration_finish();
  }
}

