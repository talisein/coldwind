#ifndef LURKER_HXX
#define LURKER_HXX
#include <glibmm/thread.h>
#include "manager.hxx"

namespace Derp {
  class Lurker {
  public:
    Lurker();
    ~Lurker();

    void add_async(const Derp::Lurk_Data& data);
    void run();

  private:
    Lurker(const Lurker&); // evil func
    const Lurker& operator=(const Lurker&); // evil func

    void add(const Derp::Lurk_Data&);
    Glib::Mutex m_list_lock;
    std::list<Derp::Lurk_Data> m_list;
    std::list<Derp::Lurk_Data>::iterator iter;

    int total_downloaded;
    void iteration_next();
    void iteration_finish(int num_downloaded, const Derp::Lurk_Data& request);

    sigc::connection m_manager_connection;
    Derp::Manager m_manager;
  };
}

#endif
