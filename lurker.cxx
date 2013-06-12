#include "lurker.hxx"
#include <iostream>
#include <glibmm/thread.h>

Derp::Lurker::Lurker(const std::shared_ptr<Manager>& manager) :
    m_manager(manager)
{
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
  if (!m_manager->download_async(*iter, std::bind(&Derp::Lurker::manager_cb, this, std::placeholders::_1))) {
    std::cerr << "Error: Lurker tried to start a download of thread, but the manager is busy." << std::endl;
    iteration_finish(0);
  } 
}

void 
Derp::Lurker::manager_cb(const std::shared_ptr<const ManagerResult>& result) {
    switch(result->state) {
        case ManagerResult::HASHING:
        case ManagerResult::LURKING:
        case ManagerResult::PARSING:
            break;
        case ManagerResult::DOWNLOADING:
            if (result->had_error) {
                download_error(result->error_code);
            }
            break;
        case ManagerResult::DONE:
            if (result->had_error) {
                download_error(result->error_code);
            } else {
                downloads_finished(result->num_downloaded, result->request);
            }
            break;
        case ManagerResult::ERROR:
            // Parsing error
            if (result->had_error) {
                download_error(result->error_code);
            } else {

            }
            break;
    }
}

void Derp::Lurker::download_error(const Derp::Error& error) {
  auto iter_copy(iter);
  switch (error) {
  case THREAD_404:
	  iter->mark404();
	  std::cout << "Lurker caught thread 404 on " << iter->getUrl() << std::endl;
	  iteration_finish(0);
	  break;
  case DUPLICATE_FILE:
  case IMAGE_CURL_ERROR:
	  break;
  case THREAD_CURL_ERROR:
  case THREAD_PARSE_ERROR:
	  std::cout << "Lurker caught a thread error." << std::endl;
	  iteration_finish(0);
  default:
	  std::cout << "Lurker caught unknown error." << std::endl;
	  iteration_finish(0);
    break;
  }
}

void Derp::Lurker::downloads_finished(int num_downloaded, const Derp::Request&) {
	iteration_finish(num_downloaded);
}

void Derp::Lurker::iteration_finish(int num_downloaded) {
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
