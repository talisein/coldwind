#include "lurker.hxx"
#include <iostream>
#include <glibmm/thread.h>

Derp::Lurker::Lurker()
{
  m_manager_connection = m_manager.signal_all_downloads_finished.connect( sigc::mem_fun(*this, &Derp::Lurker::iteration_finish) );
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
  if (!m_manager.download_async(*iter)) {
    std::cerr << "Error: Lurker tried to start a download of thread, but the manager is busy." << std::endl;
    iteration_finish(0, *iter);
  } 
}

void Derp::Lurker::iteration_finish(int num_downloaded, const Derp::Request&) {
  iter->minutes -= 1;
  total_downloaded += num_downloaded;

  iter++;
  if (iter == m_list.end()) {

    std::cout << "Lurker downloaded " << total_downloaded << " images total\n";
    std::cout << "There were " << m_list.size() << " threads being monitored, now only ";

    m_list.remove_if([](Request data) { return data.minutes <= 0; });
    std::cout << m_list.size() << " remain." << std::endl;

    m_list_lock.unlock();
  } else {
    iteration_next();
  }
}
