#include "downloader.hxx"
#include <iostream>
#include <glibmm/thread.h>
#include <glibmm/miscutils.h>
#include <iomanip>
#include "config.h"

Derp::Downloader::Downloader() : m_threadPool(4),
                                 m_curlm(curl_multi_init()),
                                 socket_info_cache_(0),
                                 bad_socket_infos_(0)
{
	if (!m_curlm) {
		std::cerr << "Error: Could not initialize the curl multi interface"
		          << std::endl;
		exit(EXIT_FAILURE);
	}

	CURLMcode code = curl_multi_setopt(m_curlm, CURLMOPT_SOCKETFUNCTION,
	                                   &Derp::curl_socket_cb);
	if (code != CURLM_OK) {
		std::cerr << "Error: While setting curl multi socket function: "
		          << curl_multi_strerror(code) << std::endl;
		exit(EXIT_FAILURE);
	}

	code = curl_multi_setopt(m_curlm, CURLMOPT_SOCKETDATA, this);
	if (code != CURLM_OK) {
		std::cerr << "Error: While setting curl multi socket function data: "
		          << curl_multi_strerror(code) << std::endl;
		exit(EXIT_FAILURE);
	}

	code = curl_multi_setopt(m_curlm, CURLMOPT_TIMERFUNCTION,
	                         &Derp::curl_timer_cb);
	if (code != CURLM_OK) {
		std::cerr << "Error: While setting curl multi timer function: "
		          << curl_multi_strerror(code) << std::endl;
		exit(EXIT_FAILURE);
	}

	code = curl_multi_setopt(m_curlm, CURLMOPT_TIMERDATA, this);
	if (code != CURLM_OK) {
		std::cerr << "Error: While setting curl multi timer function data: "
		          << curl_multi_strerror(code) << std::endl;
		exit(EXIT_FAILURE);
	}

	for ( int i = 0; i < COLDWIND_CURL_CONNECTIONS; i++ ) {
		CURL* curl = curl_easy_init();
		if (curl) {
			m_curl_queue.push(curl);
		} else {
			std::cerr << "Error: Couldn't create a curl handle! "
			          << "Out of memory already?" << std::endl;
			break;
		}
	}
}

Derp::Downloader::~Downloader() {
	Glib::Mutex::Lock lock(curl_mutex);
	CURLMcode code;

	while ( !m_curl_queue.empty() ) {
		curl_easy_cleanup( m_curl_queue.front() );
		m_curl_queue.pop();
	}

	for ( auto iter = m_fos_map.begin(); iter != m_fos_map.end();) {
		code = curl_multi_remove_handle( m_curlm, iter->first );
		if (G_UNLIKELY( code != CURLM_OK )) {
			std::cerr << "Error: While removing active handles from curl multi: "
			          << curl_multi_strerror(code) << std::endl;
		}
		curl_easy_cleanup( iter->first );
		iter->second->close();
		m_fos_map.erase(iter++);
	}

	code = curl_multi_cleanup( m_curlm );
	if (code != CURLM_OK) {
		std::cerr << "Error: While cleaning up curlm: "
		          << curl_multi_strerror(code) << std::endl;
	}

	for ( auto iter = m_file_map.begin(); iter != m_file_map.end();) {
		Glib::ustring name = iter->second->get_parse_name();
		iter->second->remove();
		m_file_map.erase(iter++);
		std::cerr << "Incomplete file " << name << " deleted." << std::endl;
	}

	for ( auto info : socket_info_cache_ )
		delete info;

	for ( auto info : bad_socket_infos_ )
		delete info;
}

static bool check_curl_code(CURLcode code, const std::string& msg) {
	if (code != CURLE_OK) {
		std::cerr << "Error: " << msg << " : " << curl_easy_strerror(code)
		          << std::endl;
		return false;
	} else {
		return true;
	}
}

static void curl_statistics_check_code(const CURLcode& code) {
	if (code != CURLE_OK) {
		std::cerr << "Error: While collecting statistics on a completed"
		          << " download: " << curl_easy_strerror(code) << std::endl;
	}
}

int Derp::downloader_progress_callback(void *clientp,
                                       double dltotal,
                                       double dlnow,
                                       double,         // ultotal
                                       double) {       // ulnow
	if (dltotal > 0.0) {
		Derp::Progress_Info* progress_info = static_cast<Derp::Progress_Info*>(clientp);


		if ( dlnow != progress_info->last ) {
			double diff = dlnow - progress_info->last;
			double scalar = progress_info->expected / dltotal;
			progress_info->downloader->increment_progress(diff*scalar);
			progress_info->last = dlnow;
		}
	}
	

	return 0; // Nonzero aborts transfer
}

static void progress_cleanup(CURL* curl) {
	char* progressdata = NULL;
	Derp::Progress_Info* progress_info;
	
	curl_easy_getinfo(curl, CURLINFO_PRIVATE, &progressdata);
	progress_info = static_cast<Derp::Progress_Info*>(static_cast<void*>(progressdata));
delete progress_info;
}

/*
  Called by curl, so it must be in a thread already
*/
int Derp::curl_socket_cb(CURL *easy,      /* easy handle */
                         curl_socket_t s, /* socket */
                         int action,      /* see values below */
                         void *userp,     /* private callback pointer */
                         void *socketp)   /* private socket pointer */
{
	Socket_Info* info = static_cast<Socket_Info*>(socketp);
	Downloader* downloader = static_cast<Downloader*>(userp);
	downloader->ASSERT_LOCK("curl_socket_cb");

	if (action == CURL_POLL_REMOVE) {
		downloader->curl_remsock(info, s);
	} else {
		if (!info) {
			downloader->curl_addsock(s, easy, action);
		} else {
			downloader->curl_setsock(info, s, easy, action);
		}
	}
	return 0;
}


void Derp::Downloader::collect_statistics(CURL* curl) {
	CURLcode code;
	double total, starttransfer, size, speed;
	long connects, redirects;

	code = curl_easy_getinfo(curl, CURLINFO_NUM_CONNECTS, &connects);
	curl_statistics_check_code(code);
	code = curl_easy_getinfo(curl, CURLINFO_REDIRECT_COUNT, &redirects);
	curl_statistics_check_code(code);
	code = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
	curl_statistics_check_code(code);
	code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &size);
	curl_statistics_check_code(code);
	m_total_bytes += size;
	code = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &speed);
	curl_statistics_check_code(code);
	code = curl_easy_getinfo(curl, CURLINFO_STARTTRANSFER_TIME, &starttransfer);
	curl_statistics_check_code(code);

	std::cout << std::setw(6) << std::setprecision(5) << size / 1000.0
	          << " kB in " << std::setw(6) << std::setprecision(3) << total
	          << " s [" << std::setw(6) << std::setprecision(5) << speed/1000.0
	          << " kB/s] {Estbl: " << std::setw(6) << std::setprecision(3)
	          << starttransfer << " s} " << connects << "nc " << redirects
	          << " rd (Total: ";

	if (m_total_bytes > 1000 * 1000)
		std::cout << std::setw(6) << std::setprecision(4)
		          << m_total_bytes / 1000000.0 << " MB";
	else
		std::cout << std::setw(6) << std::setprecision(4)
		          << m_total_bytes / 1000.0 << " kB";

	std::cout << ", Ttl Avg Spd: " << std::setw(8) << std::setprecision(7)
	          << (m_total_bytes / 1000.0) / m_timer.elapsed() << " kB/s)";

	std::cout << " AS: " << active_sockets_.size();
	std::cout << " SIC: " << socket_info_cache_.size();
	std::cout << " ASI: " << active_socket_infos_.size();
	std::cout << " BSI: " << bad_socket_infos_.size();
	std::cout << std::endl;
}

bool Derp::Downloader::finish_file_operations(CURL* curl, bool download_error) {
	bool finished_ok = true;
	ASSERT_LOCK("finish_file_operations");

	if (G_UNLIKELY(m_fos_map.count(curl) != 1 && m_file_map.count(curl) != 1)) {
		std::cerr << "Error: Got a curl reference that is not in our private "
		          << "map." << std::endl;
		finished_ok = false;
	} else {
		const auto fos_iter = m_fos_map.find(curl);
		const auto file_iter = m_file_map.find(curl);
		try {
			fos_iter->second->close();
		} catch (Gio::Error e) {
			std::cerr << "Error: While closing "
			          << file_iter->second->get_parse_name() << " : "
			          << e.what() << std::endl;
		}
		m_fos_map.erase(fos_iter);

		try {
			if (download_error) {
				file_iter->second->remove();
			} else {
				Glib::ustring name(file_iter->second->get_parse_name());
				name.erase(name.rfind(COLDWIND_PARTIAL_FILENAME_SUFFIX));
				const auto new_filename = Gio::File::create_for_parse_name(name);
				Gio::FileCopyFlags flags = Gio::FILE_COPY_OVERWRITE;
				flags |= Gio::FILE_COPY_NOFOLLOW_SYMLINKS;
				file_iter->second->move(new_filename, flags);
			}
		} catch (Gio::Error e) {
			std::cerr << "Error: While renaming file: " << e.what() << std::endl;
			finished_ok = false;
			try {
				file_iter->second->remove();
			} catch (Gio::Error err) {
				std::cerr << "Error: Could not clean up "
				          << file_iter->second->get_parse_name() << " : "
				          << err.what() << std::endl;
			}
		}
		m_file_map.erase(file_iter);
	}
	return finished_ok;
}

void Derp::Downloader::curl_check_info() {
	int msgs_left;
	CURL* curl;
	CURLcode code;
	CURLMcode mcode;
	CURLMsg* msg;
	bool download_error = false;
	bool file_ok;

	ASSERT_LOCK("curl_check_info");

	do { // while(msg != NULL && msgs_left > 0)
		msg = curl_multi_info_read(m_curlm, &msgs_left);
		if (msg) {
			switch(msg->msg) {
			case CURLMSG_DONE:
				curl = msg->easy_handle;
				code = msg->data.result;
				if (!check_curl_code(code, "Image download finished with error"))
					download_error = true;

				mcode = curl_multi_remove_handle(m_curlm, curl);
				if (G_UNLIKELY(mcode != CURLM_OK)) {
					std::cerr << "Error: While removing curl handle from multi: "
					          << curl_multi_strerror(mcode) << std::endl;
				}

				file_ok = finish_file_operations(curl, download_error);


				progress_cleanup(curl);

				if (G_LIKELY(!download_error && file_ok)) {
					collect_statistics(curl);
					signal_download_finished();
				} else {
					signal_download_error();
				}

				start_new_download(curl);
				break;
			case CURLMSG_NONE:
				std::cerr << "Warning: Unexpected MSG_NONE from curl info read."
				          << std::endl;
				break;
			case CURLMSG_LAST:
				std::cerr << "Warning: Unexpected MSG_LAST from curl info read."
				          << std::endl;
				break;
			default:
				std::cerr << "Warning: Unexpected message " << msg->msg
				          << " from curl info read." << std::endl;
			}
		}
	} while(msg != NULL && msgs_left > 0);
}

void Derp::Downloader::start_new_download(CURL* curl) {
	std::list<Derp::Image>::iterator iter;
	ASSERT_LOCK("start_new_download");

	iter = m_imgs.begin();
	if (iter != m_imgs.end()) {
		curl_easy_reset(curl);
		const bool setup_ok = curl_setup(curl, *iter);
		if (G_LIKELY( setup_ok )) {
			CURLMcode code = curl_multi_add_handle(m_curlm, curl);
			if (G_LIKELY( code == CURLM_OK )) {
				m_imgs.erase(iter);
			} else {
				std::cerr << "Error: While adding a new download to multi "
				          << "handle: " << curl_multi_strerror(code) << std::endl;
				curl_easy_reset(curl);
				m_curl_queue.push(curl);
			}
		}
	} else {
		curl_easy_reset(curl);
		m_curl_queue.push(curl);
	}
}

inline void Derp::Downloader::ASSERT_LOCK(const std::string& func) const {
	if(G_UNLIKELY(curl_mutex.trylock())) {
		std::cout << "Assertion error: " << func
		          << " called and curl_mutex is not locked!" << std::endl;
		curl_mutex.unlock();
	}
}

/*
  Called by curl, so it must be in a thread already
*/
int Derp::curl_timer_cb(CURLM*,    /* multi handle */
                        long timeout_ms,
                        void *userp) /* private callback pointer */
{
	Downloader* downloader = static_cast<Downloader*>(userp);
	unsigned int timeout = static_cast<unsigned int>(timeout_ms);

	downloader->ASSERT_LOCK("curl_timer_cb");

	if (downloader->m_timeout_connection.connected())
		downloader->m_timeout_connection.disconnect();

	if (timeout_ms == -1) {
		return 0;
	} else if (timeout_ms == 0) {
		downloader->curl_timeout_expired_cb();
	} else {
		auto s = sigc::mem_fun(*downloader, &Downloader::curl_timeout_expired_cb);
		downloader->m_timeout_connection = Glib::signal_timeout().connect(s, timeout);
	}

	return 0;
}

/*
  Called by Glib::MainLoop
*/
bool Derp::Downloader::curl_timeout_expired_cb() {
	const auto s = sigc::mem_fun(*this, &Derp::Downloader::curl_timeout_expired );
	m_threadPool.push( s );
	return false;
}

void Derp::Downloader::curl_timeout_expired() {
	Glib::Mutex::Lock lock(curl_mutex);
	CURLMcode code = curl_multi_socket_action(m_curlm, CURL_SOCKET_TIMEOUT, 0,
	                                          &m_running_handles);
	if (G_UNLIKELY( code != CURLM_OK )) {
		std::cerr << "Error: Curl socket action error from timeout: "
		          << curl_multi_strerror(code) << std::endl;
	}

	curl_check_info();
}

/*
  Called from Glib::MainLoop when socket s has activity
*/
bool Derp::Downloader::curl_event_cb(Glib::IOCondition condition,
                                     curl_socket_t s) {
	int action = (condition & Glib::IO_IN ? CURL_CSELECT_IN : 0) |
		(condition & Glib::IO_OUT ? CURL_CSELECT_OUT : 0);
	const auto slot = sigc::mem_fun(*this, &Derp::Downloader::curl_event);
	m_threadPool.push( sigc::bind(slot, action, s) );

	return true;
}

/*
  A new thread started by the MainLoop above because socket s has
  activity. We must take the lock.
*/
void Derp::Downloader::curl_event(int action, curl_socket_t s) {
	Glib::Mutex::Lock lock(curl_mutex);
	CURLMcode code;

	code = curl_multi_socket_action(m_curlm, s, action, &m_running_handles);
	if (G_UNLIKELY( code != CURLM_OK )) {
		std::cerr << "Error: Curl socket action error from event: "
		          << curl_multi_strerror(code) << std::endl;
	}
	curl_check_info();
	if (m_running_handles == 0) {
		if (m_timeout_connection.connected())
			m_timeout_connection.disconnect();
	}
}

void Derp::Downloader::curl_setsock(Socket_Info* info, curl_socket_t s,
                                    CURL*, int action) {
	Glib::IOCondition condition = Glib::IO_IN ^ Glib::IO_IN;
	ASSERT_LOCK("curl_setsock");

	if (action & CURL_POLL_IN)
		condition = Glib::IO_IN;
	if (action & CURL_POLL_OUT)
		condition = condition | Glib::IO_OUT;

	if (info->connection.connected())
		info->connection.disconnect();
	const auto slot = sigc::mem_fun(*this, &Derp::Downloader::curl_event_cb);
	info->connection = Glib::signal_io().connect( sigc::bind(slot, s),
	                                              info->channel,
	                                              condition );
}

void Derp::Downloader::curl_addsock(curl_socket_t s, CURL *easy, int action) {
	ASSERT_LOCK("curl_addsock");

	if( G_LIKELY( std::count(active_sockets_.begin(),
	                         active_sockets_.end(), s) == 0 )) {
		active_sockets_.push_back(s);
	} else {
		std::cerr << "Error: curl_addsock got called twice for the same socket."
		          << " I don't know what to do!" << std::endl;
	}

	Socket_Info* info;
	if (socket_info_cache_.empty()) {
		info = new Socket_Info();
	} else {
		info = socket_info_cache_.back();
		socket_info_cache_.pop_back();
	}

	if ( G_LIKELY(std::count(active_socket_infos_.begin(),
	                         active_socket_infos_.end(), info) == 0 )) {
		active_socket_infos_.push_back(info);
	} else {
		std::cerr << "Error: Somehow the info we are assigning is already active!"
		          << std::endl;
	}

#if COLDWIND_WINDOWS
	info->channel = Glib::IOChannel::create_from_win32_socket(s);
#else
	info->channel = Glib::IOChannel::create_from_fd(s);
#endif

	if ( G_UNLIKELY( !info->channel )) {
		std::cerr << "Error: Unable to create IOChannel for socket " << s
		          << std::endl;
	}

	curl_setsock(info, s, easy, action);
	CURLMcode code = curl_multi_assign(m_curlm, s, info);
	if (G_UNLIKELY( code != CURLM_OK )) {
		std::cerr << "Error: Unable to assign Socket_Info to socket : "
		          << curl_multi_strerror(code) << std::endl;
	}
}

void Derp::Downloader::curl_remsock(Socket_Info* info, curl_socket_t s) {
	ASSERT_LOCK("curl_remsock");

	if (G_UNLIKELY( !info ))
		return;

	if (G_LIKELY( std::count(active_sockets_.begin(),
	                         active_sockets_.end(), s) > 0 )) {
		if (G_LIKELY( std::count(active_socket_infos_.begin(),
		                         active_socket_infos_.end(), info) > 0 )) {
			// We SHOULD NOT call info->channel->close(). This closes curl's
			// cached connection to the server.
			info->connection.disconnect();
			info->channel.reset();
			socket_info_cache_.push_back(info);
			active_socket_infos_.remove(info);
			active_sockets_.remove(s);
		} else {
			std::cerr << "Warning: curl_remsock called on info " << info
			          << " socket " << s << ",  but that info is not active!"
			          << std::endl;
			bad_socket_infos_.push_back(info);
			// TODO: Figure out what to do with these bad boys.
		}
	} else {
		std::cerr << "Warning: curl_remsock was called twice for the same socket. "
		          << "This should be safe to ignore." << std::endl;
	}
}

/*
  Called by curl, so should be in a thread. But we don't have a
  pointer to Downloader to assert. In anycase, we're not touching
  anything that needs to be mutex'd.
*/
size_t Derp::write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	GFileOutputStream* gfos = static_cast<GFileOutputStream*>(userdata);
	gssize written = 0;
	Glib::RefPtr<Gio::FileOutputStream> p_fos = Glib::wrap(gfos, true);

	try {
		written = p_fos->write(static_cast<char*>(ptr), size*nmemb);
	} catch (Gio::Error e) {
		std::cerr << "Error: Could not write to file: " << e.what() << std::endl;
	}

	return written;
}

bool Derp::Downloader::curl_setup(CURL* curl, const Derp::Image& img) {
	const Glib::ustring url(img.getUrl());
	const std::string filename(img.getFilename());
	const std::string filepath(Glib::build_filename(m_target_dir->get_path(),
	                                                filename));
	Glib::RefPtr<Gio::File> p_file;
	Glib::RefPtr<Gio::FileOutputStream> p_fos;
	CURLcode code;
	Derp::Progress_Info* progress_info;

	try {
		if (!m_target_dir->query_exists()) {
			if (!m_target_dir->make_directory_with_parents()) {
				std::cerr << "Error: Unable to create target directory." << std::endl;
				goto on_file_failure;
			}
		}

		p_file = Gio::File::create_for_path(filepath);
		if ( p_file->query_exists() ) {
			auto info = p_file->query_info();
			if ( info->get_size() != 0 ) {
				if (m_request.useOriginalFilename()) {
					const size_t last_period = filename.find_last_of(".");
					const std::string ext(filename.substr(last_period));
					const std::string name(filename.substr(0, last_period));

					for ( guint32 i = 1; p_file->query_exists(); i++ ) {
						std::stringstream st;
						st << name << " (" << i << ")" << ext;
						auto path = Glib::build_filename(m_target_dir->get_path(),
						                                 st.str());
						p_file = Gio::File::create_for_path( path );
					}
				} else { // useOriginalFilename == false
					if (p_file->query_exists()) {
						std::cerr << "Warning: File " << filepath << " already "
						          << "exists, but the hash does not match. "
						          << "Overwriting. Repeated warning messages may"
						          << "indicate filesystem corruption or a "
						          << "programming error in coldwind."
						          << std::endl;
					}
				}
			}
		}

		// At this point, p_file points to the name we're going to use
		auto name = p_file->get_parse_name().append(COLDWIND_PARTIAL_FILENAME_SUFFIX);
		p_file = Gio::File::create_for_parse_name(name);
		p_fos = p_file->replace();
	} catch (Gio::Error e) {
		switch (e.code()) {
		case Gio::Error::Code::EXISTS:
			std::cerr << "Warning: File " << filepath << " already exists."
			          << std::endl;
			goto on_file_failure;
			break;
		default:
			std::cerr << "Error creating file " << filepath << ": " << e.what()
			          << std::endl;
			goto on_file_failure;
		}
	}

	code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl URL" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Derp::write_cb);
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl WRITEFUNCTION" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, p_fos->gobj());
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl WRITEDATA" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl FAILONERROR" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl NOPROGRESS" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
	                        &downloader_progress_callback);
	if (G_UNLIKELY( !check_curl_code(code,
	                                 "While setting curl PROGRESSFUNCTION" )))
		goto on_setup_failure;
	
	progress_info = new Derp::Progress_Info{this, 0.0, img.getExpectedSize()};

	code = curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progress_info);
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl PROGRESSDATA" )))
		goto on_setup_failure;

	code = curl_easy_setopt(curl, CURLOPT_PRIVATE, progress_info);
	if (G_UNLIKELY( !check_curl_code(code, "While setting curl PRIVATE" )))
		goto on_setup_failure;

	

	// At this point, the file and curl handle have been setup successfully.
	m_fos_map.insert({curl, p_fos});
	m_file_map.insert({curl, p_file});
	return true;

 on_setup_failure:
	p_fos->close();
	// Fallthrough
 on_file_failure:
	signal_download_error();
	return false;
}

void Derp::Downloader::download_imgs_multi() {
	Glib::Mutex::Lock lock(curl_mutex);
	CURLMcode m_code;
	double bytes = 0.0;
	expected_bytes_ = 0.0;
	
	std::for_each(m_imgs.begin(), 
	              m_imgs.end(), 
	              [&bytes] (const Derp::Image& img) 
	              { bytes += img.getExpectedSize(); });
	expected_bytes_ = bytes;

	while ( !m_curl_queue.empty() ) {
		auto it = m_imgs.begin();
		if (it == m_imgs.end()) {
			break;
		} else {
			CURL* curl = m_curl_queue.front(); m_curl_queue.pop();
			bool setup_ok = curl_setup(curl, *it);
			if (G_UNLIKELY( !setup_ok )) {
				curl_easy_reset(curl);
				m_curl_queue.push(curl);
			} else {
				m_code = curl_multi_add_handle(m_curlm, curl);
				if (G_UNLIKELY( m_code != CURLM_OK )) {
					std::cerr << "Error: While adding curl to multi handle: "
					          << curl_multi_strerror(m_code) << std::endl;
					curl_easy_reset(curl);
					m_curl_queue.push(curl);
				} else {
					m_imgs.erase(it);
				}
			}
		}
	}
}

void Derp::Downloader::download_async(const std::list<Derp::Image>& imgs,
                                      const Derp::Request& request) {
	m_request = request;
	m_imgs = imgs;
	m_target_dir = request.getDirectory();

	m_timer.reset();
	m_timer.start();
	m_total_bytes = 0;
	progress_bytes_ = 0.0;

	const auto slot = sigc::mem_fun(*this, &Derp::Downloader::download_imgs_multi);
	m_threadPool.push( slot );
}
