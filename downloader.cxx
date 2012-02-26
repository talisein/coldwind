#include "downloader.hxx"
#include <iostream>
#include <glibmm/thread.h>
#include <iomanip>
#include "config.h"

Derp::Downloader::Downloader() : m_threadPool(4), 
				 m_curlm(curl_multi_init()),
				 socket_info_cache_(0),
				 bad_socket_infos_(0)
{
  if (!m_curlm) {
    std::cerr << "Error: Could not initialize the curl multi interface" << std::endl;
    exit(EXIT_FAILURE);
  }

  CURLMcode code = curl_multi_setopt(m_curlm, CURLMOPT_SOCKETFUNCTION, &Derp::curl_socket_cb);
  if (code != CURLM_OK) {
    std::cerr << "Error: While setting curl multi socket function: " << curl_multi_strerror(code) << std::endl;
    exit(EXIT_FAILURE);
  }

  code = curl_multi_setopt(m_curlm, CURLMOPT_SOCKETDATA, this);
  if (code != CURLM_OK) {
    std::cerr << "Error: While setting curl multi socket function data: " << curl_multi_strerror(code) << std::endl;
    exit(EXIT_FAILURE);
  }

  code = curl_multi_setopt(m_curlm, CURLMOPT_TIMERFUNCTION, &Derp::curl_timer_cb);
  if (code != CURLM_OK) {
    std::cerr << "Error: While setting curl multi timer function: " << curl_multi_strerror(code) << std::endl;
    exit(EXIT_FAILURE);
  }

  code = curl_multi_setopt(m_curlm, CURLMOPT_TIMERDATA, this);
  if (code != CURLM_OK) {
    std::cerr << "Error: While setting curl multi timer function data: " << curl_multi_strerror(code) << std::endl;
    exit(EXIT_FAILURE);
  }

  for ( int i = 0; i < COLDWIND_CURL_CONNECTIONS; i++ ) {
    CURL* curl = curl_easy_init();
    if (curl) {
      m_curl_queue.push(curl);
    } else {
      std::cerr << "Error: Couldn't create a curl handle! Out of memory already?" << std::endl;
      break;
    }
  }
}

Derp::Downloader::~Downloader() {
  CURLMcode code = curl_multi_cleanup( m_curlm );
  if (code != CURLM_OK) {
    std::cerr << "Error: While cleaning up curl: " << curl_multi_strerror(code) << std::endl;
  }

  while ( !m_curl_queue.empty() ) {
    curl_easy_cleanup( m_curl_queue.front() );
    m_curl_queue.pop();
  }

  // TODO: Cleanup our socket_info_cache_, etc.
}

/*
  Called by curl, so it must be in a thread already
 */
int Derp::curl_socket_cb(CURL *easy, /* easy handle */   
			 curl_socket_t s, /* socket */   
			 int action, /* see values below */   
			 void *userp, /* private callback pointer */   
			 void *socketp) /* private socket pointer */
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

static void curl_statistics_check_code(const CURLcode& code) {
  if (code != CURLE_OK) {
    std::cerr << "Error: While collecting statistics on a completed download: " << curl_easy_strerror(code) << std::endl;
  }
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

  std::cout << std::setw(6) << std::setprecision(5) << size / 1000.0 << " kB in " << std::setw(6) << std::setprecision(3) << total << " s [" << std::setw(6) << std::setprecision(5) << speed/1000.0 << " kB/s] {Estbl: " << std::setw(6) << std::setprecision(3) << starttransfer << " s} " << connects << "nc " << redirects << " rd (Total: ";
  
  if (m_total_bytes > 1000 * 1000) 
    std::cout << std::setw(6) << std::setprecision(4) << m_total_bytes / 1000000.0 << " MB";
  else 
    std::cout << std::setw(6) << std::setprecision(4) << m_total_bytes / 1000.0 << " kB";

  std::cout << ", Ttl Avg Spd: " << std::setw(8) << std::setprecision(7) << (m_total_bytes / 1000.0) / m_timer.elapsed() << " kB/s)";

  std::cout << " AS: " << active_sockets_.size();
  std::cout << " SIC: " << socket_info_cache_.size();
  std::cout << " ASI: " << active_socket_infos_.size();
  std::cout << " BSI: " << bad_socket_infos_.size();
  std::cout << std::endl;
}

void Derp::Downloader::curl_check_info() {
  int msgs_left;
  CURL* curl;
  CURLcode code;
  CURLMcode mcode;
  CURLMsg* msg;
  std::list<Derp::Image>::iterator iter;
  bool setup_ok;
  bool download_error = false;

  ASSERT_LOCK("curl_check_info");
  do {
    msg = curl_multi_info_read(m_curlm, &msgs_left);
    if (msg) {
      switch(msg->msg) {
      case CURLMSG_DONE:
	curl = msg->easy_handle;
	code = msg->data.result;
	if (code != CURLE_OK) {
	  std::cerr << "Error: Image download finished with error: " << curl_easy_strerror(code) << std::endl;
	  download_error = true;
	}

	if (m_fos_map.count(curl) != 1) {
	  std::cerr << "Error: Got a curl reference that is not in our private map." << std::endl;
	} else {
	  auto iter = m_fos_map.find(curl);
	  try {
	    iter->second->close();
	  } catch (Gio::Error e) {
	    std::cerr << "Error: While closing a file: " << e.what() << std::endl;
	  } 
	  m_fos_map.erase(iter);
	}

	collect_statistics(curl);

	mcode = curl_multi_remove_handle(m_curlm, curl);
	if (mcode != CURLM_OK) {
	  std::cerr << "Error: While removing curl handle from multi: " << curl_multi_strerror(mcode) << std::endl;
	}

	if (!download_error)
	  signal_download_finished();
	else
	  signal_download_error();

	iter = m_imgs.begin();
	if (iter != m_imgs.end()) {
	  curl_easy_reset(curl);
	  setup_ok = curl_setup(curl, *iter);
	  if (setup_ok) {
	    curl_multi_add_handle(m_curlm, curl);
	  }
	  m_imgs.erase(iter);
	} else {
	  curl_easy_reset(curl);
	  m_curl_queue.push(curl);
	}
	break;
      default:
	std::cerr << "Warning: Unexpected message received from curl info read." << std::endl;
      }
    }
  } while(msg != NULL && msgs_left > 0);
}


void Derp::Downloader::ASSERT_LOCK(std::string func) const {
  if(curl_mutex.trylock()) {
    std::cout << "Assertion error: " << func << " called and curl_mutex is not locked!" << std::endl;
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
    downloader->m_timeout_connection = Glib::signal_timeout().connect( sigc::mem_fun(*downloader, &Derp::Downloader::curl_timeout_expired_cb), timeout );
  }
  return 0;
}

/* 
   Called by Glib::MainLoop
 */

bool Derp::Downloader::curl_timeout_expired_cb() {
  m_threadPool.push( sigc::mem_fun(*this, &Derp::Downloader::curl_timeout_expired ) );
  return false;
}

void Derp::Downloader::curl_timeout_expired() {
  Glib::Mutex::Lock lock(curl_mutex);
  CURLMcode code = curl_multi_socket_action(m_curlm, CURL_SOCKET_TIMEOUT, 0, &m_running_handles);
  if (code != CURLM_OK) {
    std::cerr << "Error: Curl socket action error from timeout: " << curl_multi_strerror(code) << std::endl;
  }

  curl_check_info();
}

/* 
   Called from Glib::MainLoop when socket s has activity
*/
bool Derp::Downloader::curl_event_cb(Glib::IOCondition condition, curl_socket_t s) {
  int action = (condition & Glib::IO_IN ? CURL_CSELECT_IN : 0) |
    (condition & Glib::IO_OUT ? CURL_CSELECT_OUT : 0);
  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::curl_event), action, s) );
  
  return true;
}

void Derp::Downloader::curl_event(int action, curl_socket_t s) {
  Glib::Mutex::Lock lock(curl_mutex);
  CURLMcode code;

  code = curl_multi_socket_action(m_curlm, s, action, &m_running_handles);
  if (code != CURLM_OK) {
    std::cerr << "Error: Curl socket action error from event: " << curl_multi_strerror(code) << std::endl;
  }
  curl_check_info();
  if (!m_running_handles) {
    if (m_timeout_connection.connected())
      m_timeout_connection.disconnect();
    // Here we used to return false to disconnect this socket from the GIOChannel
  }
}

void Derp::Downloader::curl_setsock(Socket_Info* info, curl_socket_t s, CURL*, int action) {
  Glib::IOCondition condition = Glib::IO_IN ^ Glib::IO_IN;

  ASSERT_LOCK("curl_setsock");

  if (action & CURL_POLL_IN)
    condition = Glib::IO_IN;
  if (action & CURL_POLL_OUT)
    condition = condition | Glib::IO_OUT;

  if (info->connection.connected())
    info->connection.disconnect();
  info->connection = Glib::signal_io().connect( sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::curl_event_cb), s), info->channel, condition );
}

void Derp::Downloader::curl_addsock(curl_socket_t s, CURL *easy, int action) {
  ASSERT_LOCK("curl_addsock");

  if( std::count(active_sockets_.begin(), active_sockets_.end(), s) == 0 ) {
    active_sockets_.push_back(s);
  } else {
    std::cerr << "Error: curl_addsock got called twice for the same socket. I don't know what to do!" << std::endl;
  }
  
  Socket_Info* info;
  if (socket_info_cache_.empty()) {
    info = new Socket_Info();
  } else {
    info = socket_info_cache_.back();
    socket_info_cache_.pop_back();
  }
  
  if ( std::count(active_socket_infos_.begin(), active_socket_infos_.end(), info) == 0 ) {
    active_socket_infos_.push_back(info);
  } else {
    std::cerr << "Error: Somehow the info we are assigning is already active!" << std::endl;
  }
    
  #if COLDWIND_WINDOWS
  info->channel = Glib::IOChannel::create_from_win32_socket(s);
  #else
  info->channel = Glib::IOChannel::create_from_fd(s);
  #endif
  curl_setsock(info, s, easy, action);
  curl_multi_assign(m_curlm, s, info);
}

void Derp::Downloader::curl_remsock(Socket_Info* info, curl_socket_t s) {
  ASSERT_LOCK("curl_remsock");

  if (!info)
    return;
  if ( std::count(active_sockets_.begin(), active_sockets_.end(), s) > 0) {
    if ( std::count(active_socket_infos_.begin(), active_socket_infos_.end(), info) > 0) {
      // We SHOULD NOT call info->channel->close(). This closes curl's
      // cached connection to the server.
      info->connection.disconnect();
      info->channel.reset();
      socket_info_cache_.push_back(info);
      active_socket_infos_.remove(info); 
      active_sockets_.remove(s);
    } else {
      std::cerr << "Warning: curl_remsock called on info " << info << " socket " << s << ",  but that info is not active!" << std::endl;
      bad_socket_infos_.push_back(info);
      // TODO: Figure out what to do with these bad boys.
    }
  } else {
    std::cerr << "Warning: curl_remsock was called twice for the same socket. This should be safe to ignore." << std::endl;
  }
}

/* 
   Called by curl, so should be in a thread. But we don't have a
   pointer to Downloader to assert.
 */
size_t Derp::write_cb(void *ptr, size_t size, size_t nmemb, void *userdata)
{
  GFileOutputStream* gfos = static_cast<GFileOutputStream*>(userdata);
  gssize written = 0;
  Glib::RefPtr<Gio::FileOutputStream> p_fos = Glib::wrap(gfos, true);
  // std::string str(static_cast<char*>(ptr), size*nmemb);

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
  const std::string filepath(Glib::build_filename(m_target_dir->get_path(), filename));

  Glib::RefPtr<Gio::FileOutputStream> p_fos;
  CURLcode code;

  try {
    if (!m_target_dir->query_exists()) {
      if (!m_target_dir->make_directory_with_parents()) {
	std::cerr << "Unable to create target directory." << std::endl;
	goto on_file_failure;
      }
    }

    Glib::RefPtr<Gio::File> p_file = Gio::File::create_for_path(filepath);
    if ( p_file->query_exists() ) {
      auto info = p_file->query_info();
      if ( info->get_size() != 0 ) {
	for ( int i = 1; i < 999; i++ ) {
	  if(request_.useOriginalFilename() && p_file->query_exists()) {
	    std::string ext(filename.substr(filename.find_last_of(".")));
	    std::string name(filename.substr(0, filename.find_last_of(".")));
	    std::stringstream st;
	    st << name << " (" << i << ")" << ext;
	    p_file = Gio::File::create_for_path(Glib::build_filename(m_target_dir->get_path(), st.str()));
	  } else {
	    break;
	  }
	} // for
	p_fos = p_file->create_file();
      } else { // File exists, but is 0 size. Replace it.
	p_fos = p_file->replace();
      }
    }
  } catch (Gio::Error e) {
    switch (e.code()) {
    case Gio::Error::Code::EXISTS:
      std::cerr << "Warning: File " << filepath << " already exists. Skipping." << std::endl;
      //std::cerr << "This means the file already existing does not match the MD5 hash of what's on 4chan. If Coldwind is the only program saving to this directory, then it's downloads are corrupted somehow. Complain on /g/ or something." << std::endl;
      std::cerr << "If this is a transient error (i.e. it goes away on its own the next time you hit the download button), there is no real problem. The files aren't being flushed timely, or something..." << std::endl;
      goto on_file_failure;
      break;
    default:
      std::cerr << "Error creating file " << filepath << ": " << e.what() << std::endl;
      goto on_file_failure;
    }
  }
  
  code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  if (code != CURLE_OK) {
    std::cerr << "Error: While setting curl url to " << url << ": " << curl_easy_strerror(code) << std::endl; 
    goto on_setup_failure;
  }
  
  code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Derp::write_cb);
  if (code != CURLE_OK) {
    std::cerr << "Error: While setting curl write function: " << curl_easy_strerror(code) << std::endl; 
    goto on_setup_failure;
  }
  
  code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, p_fos->gobj());
  if (code != CURLE_OK) {
    std::cerr << "Error: While setting curl write data: " << curl_easy_strerror(code) << std::endl; 
    goto on_setup_failure;
  }

  code = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  if (code != CURLE_OK) {
    std::cerr << "Error: While setting curl to fail on error: " << curl_easy_strerror(code) << std::endl; 
    goto on_setup_failure;
  }
  

  m_fos_map.insert({curl, p_fos});
  return true;

 on_setup_failure:
  p_fos->close();
 on_file_failure:
  signal_download_error();
  return false;
}

void Derp::Downloader::download_imgs_multi() {
  Glib::Mutex::Lock lock(curl_mutex);
  CURLMcode m_code;

  while ( !m_curl_queue.empty() ) {
    auto it = m_imgs.begin();
    if (it == m_imgs.end()) {
      break;
    } else {
      CURL* curl = m_curl_queue.front(); m_curl_queue.pop();
      bool setup_ok = curl_setup(curl, *it);
      if (!setup_ok) {
	curl_easy_reset(curl);
	m_curl_queue.push(curl);
      } else {
	m_code = curl_multi_add_handle(m_curlm, curl); 
	if (m_code != CURLM_OK) {
	  std::cerr << "Error: While adding curl handle to curl multi handle: " << curl_multi_strerror(m_code) << std::endl; 
	  curl_easy_reset(curl);
	  m_curl_queue.push(curl);
	}
      } 
    }
    m_imgs.erase(it);
  }
}

void Derp::Downloader::download_async(const std::list<Derp::Image>& imgs, const Derp::Request& request) {
  request_ = request;
  m_imgs = imgs;
  m_target_dir = request.getDirectory();
  m_timer.reset();
  m_timer.start();
  m_total_bytes = 0;

  m_threadPool.push( sigc::mem_fun(*this, &Derp::Downloader::download_imgs_multi) );
}

/*
void Derp::Downloader::download_async_easy(const std::list<Derp::Image>& imgs, const Derp::Request& request) {
  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::download_imgs), imgs, request));
}

void Derp::Downloader::download_imgs(const std::list<Derp::Image>& imgs, const Derp::Request& request) {
  for( auto it = imgs.begin(); it != imgs.end(); it++) {
    m_threadPool.push(sigc::bind(sigc::mem_fun(*this, &Derp::Downloader::download_url), it->getUrl(), request ));
  }
}

void Derp::Downloader::download_url(const Glib::ustring& url, const Derp::Request& request) {
  CURL *curl = curl_easy_init();
  CURLcode res, code;
  Glib::ustring filename(url);
  filename.erase(0, url.find_last_of("/") + 1);
  Glib::RefPtr<Gio::FileOutputStream> p_fos;
  try {
    Glib::RefPtr<Gio::File> p_file = Gio::File::create_for_path(Glib::build_filename(request.target_directory->get_path(), filename));
    p_fos = p_file->create_file();
  } catch (Gio::Error e) {
    std::cerr << "Failure creating file " << Glib::build_filename(request.target_directory->get_path(), filename) << std::endl;
    std::cerr << "\tThere's probably a file named that already. Skipping." << std::endl;
    signal_download_finished(); // Call so we don't hang the progress bar? 

    curl_easy_cleanup(curl);
    return;  
  }
  

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Derp::write_cb);
    if (code != CURLE_OK) {
      std::cerr << "Error: While setting curl write function: " << curl_easy_strerror(code) << std::endl; 
      curl_easy_cleanup(curl);
      signal_download_finished();
      return;
    }
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, p_fos->gobj());
    
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "An error occured with curl\n";
    }
    curl_easy_cleanup(curl);
  }

  signal_download_finished();
}
*/
