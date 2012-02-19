#include "lurker.hxx"
#include <iostream>
#include <glibmm/thread.h>

Derp::Lurker::Lurker() 

{
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

void Derp::Lurker::run_async() {
  Glib::Thread::create( sigc::mem_fun(*this, &Derp::Lurker::run), false);
}

void Derp::Lurker::run() {
  Glib::Mutex::Lock lock(m_list_lock);
  std::cout << "The lurker is running in its own thread :)" << std::endl;  
  num_downloaded = 0;

  for( auto it = m_list.begin(); it != m_list.end(); it++ ) {
    
  }
}
