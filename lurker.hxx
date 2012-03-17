#ifndef LURKER_HXX
#define LURKER_HXX
#include <glibmm/thread.h>
#include "manager.hxx"

namespace Derp {
	class Lurker {
	public:
		Lurker();
		~Lurker();

		void add_async(const Derp::Request& data);
		void run();

	private:
		Lurker(const Lurker&); // evil func
		const Lurker& operator=(const Lurker&); // evil func

		void download_error(const Derp::Error&);

		void add(const Derp::Request&);
		Glib::Mutex m_list_lock;
		std::list<Derp::Request> m_list;
		std::list<Derp::Request>::iterator iter;

		int total_downloaded;
		void iteration_next();
		void downloads_finished(int num_downloaded, const Derp::Request&);
		void iteration_finish(int num_downloaded);

		sigc::connection m_manager_connection;
		Derp::Manager m_manager;
	};
}

#endif
