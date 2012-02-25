#ifndef HASHER_CXX
#define HASHER_CXX

#include "hasher.hxx"
#include <glibmm/thread.h>
#include <iostream>

Derp::Hasher::Hasher() : m_threadPool(8) {

}

void Derp::Hasher::hash_async(const Derp::Request& request) {
  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash), request) );
}

void Derp::Hasher::hash(const Derp::Request& request) {
  Glib::RefPtr<Gio::File> base = request.getHashDirectory();
  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory), base));

  auto thread_dir = base->get_child(Glib::filename_from_utf8(request.getThread()));
  if (thread_dir->query_exists()) {
    m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory), thread_dir) );
  }

  auto board_dir = base->get_child(Glib::filename_from_utf8(request.getBoard()));
  if (board_dir->query_exists()) {
    m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory), board_dir) );
    auto enumerator = board_dir->enumerate_children();
    for ( auto info = enumerator->next_file(); info != 0; info = enumerator->next_file()) {
      if ( info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY ) {
	m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory),
				      board_dir->get_child(info->get_name())));
      }
    }
  }
}

void Derp::Hasher::hash_directory(const Glib::RefPtr<Gio::File>& dir) {
  Glib
::RefPtr<Gio::FileEnumerator> enumerator = dir->enumerate_children();
  for(auto info = enumerator->next_file(); info != 0; info = enumerator->next_file()) {
    Gio::FileType fileType = info->get_file_type();
    if (fileType != Gio::FileType::FILE_TYPE_REGULAR)
      // TODO: What do we do about symbolic links?
      // Curl might obliterate them........
      continue;
    auto file = dir->get_child(info->get_name());
    if (!is_hashed(file->get_path())) {
      m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_file), file) );
    }
  }
  enumerator->close();
  
  while( m_threadPool.unprocessed() > 0 ) {
    Glib::Thread::yield();
  }
  signal_hashing_finished();
}

void Derp::Hasher::hash_file(const Glib::RefPtr<Gio::File>& file) {
    char* contents;
    gsize length;

    try {
      if (file->load_contents(contents, length)) {
	std::string md5hex = Glib::Checksum::compute_checksum(Glib::Checksum::ChecksumType::CHECKSUM_MD5,
							      reinterpret_cast<guchar*>(contents),
							      length);
	insert_image({file->get_path(), md5hex});
	insert_filepath(file->get_path());
	g_free(contents);
      } else {
	std::cerr << "Error: Couldn't load local file contents for hashing :( We might end up downloading something we don't need." << std::endl;
	std::cerr << "\tThe file was " << file->get_uri() << std::endl;
      }
    } catch (Gio::Error e) {
      std::cerr << "Error: While trying to load and hash " << file->get_uri() << ": " << e.what() << " -=- Code: " << e.code() << " -=-" << std::endl;
    }
}

void Derp::Hasher::insert_image(const Derp::Image& image) {
  Glib::Mutex::Lock lock(hash_mutex);
  m_image_set.insert(image);
}

void Derp::Hasher::insert_filepath(const std::string& path) {
  Glib::Mutex::Lock lock(filepath_mutex);
  m_filepath_set.insert(path);
}

bool Derp::Hasher::is_hashed(const std::string& path) const {
  Glib::Mutex::Lock lock(filepath_mutex);
  return m_filepath_set.count(path) > 0;
}

bool Derp::Hasher::is_downloaded(const Derp::Image& image) const {
  Glib::Mutex::Lock lock(hash_mutex);
  return m_image_set.count(image) > 0;
}

#endif
