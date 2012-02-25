#include "lurker.hxx"
#include <iostream>
#include <glibmm/thread.h>

Derp::Lurker::Lurker()
{
  m_manager_connection = m_manager.signal_all_downloads_finished.connect( sigc::mem_fun(*this, &Derp::Lurker::iteration_finish) );
  m_manager.signal_download_error.connect( sigc::mem_fun(*this, &Derp::Lurker::download_error) );
}

Derp::Lurker::~Lurker() {
  m_manager_connection.disconnect();
}

/*
  Since the list is accessed in a worker thread, we need to take a
  mutex before we access it. But we don't want the GUI thread to
  freeze waiting for run() to finish. So we have to make an async
  method.
 */
void Derp::Lurker::add_async(const Derp::Request& data) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Derp::Lurker::add), data), false);
}

void Derp::Lurker::add(const Derp::Request& data) {
  Glib::Mutex::Lock lock(m_list_lock);
  m_list.push_back(data);
}

void Derp::Lurker::run() {
  if (!m_list_lock.trylock()) {
    std::cerr << "Lurker got called to run(), but is still working on a previous run. Skipping." << std::endl;
    return;
  }

  total_downloaded = 0;
  iter = m_list.begin();
  if (iter != m_list.end()) {
    iteration_next();
  } else {
    m_list_lock.unlock();
  }
}

void Derp::Lurker::iteration_next() {
  std::cout << "Lurking " << iter->getUrl() << std::endl;
  if (!m_manager.download_async(*iter)) {
    std::cerr << "Error: Lurker tried to start a download of thread, but the manager is busy." << std::endl;
    iteration_finish(0, *iter);
  } 
}

void Derp::Lurker::download_error(const Derp::Error& error) {
  auto iter_copy(iter);
  switch (error) {
  case THREAD_404:
    std::cout << "Lurker caught thread 404 on " << iter->getUrl() << std::endl;
    iter++;
    m_list.erase(iter_copy);
    if (iter == m_list.end()) {
      if ( total_downloaded > 0 ) {
	std::cout << "Lurker downloaded " << total_downloaded << " images total\n";
      }

      m_list.remove_if([](const Request& data) { return data.isExpired(); });
      
      m_list_lock.unlock();
    } else {
      iteration_next();
    }
    break;
  default:
    break;
  }
}

void Derp::Lurker::iteration_finish(int num_downloaded, const Derp::Request&) {
  iter->decrementMinutes();
  total_downloaded += num_downloaded;

  iter++;
  if (iter == m_list.end()) {
    if ( total_downloaded > 0 ) {
      std::cout << "Lurker downloaded " << total_downloaded << " images total\n";
    }

    int before = m_list.size();
    m_list.remove_if([](const Request& data) { return data.isExpired(); });
    int after = m_list.size();

    if (before != after) {
      std::cout << "There were " << before << " threads being monitored, now only " 
		<< after << " remain." << std::endl;
    }

    m_list_lock.unlock();
  } else {
    iteration_next();
  }
}
