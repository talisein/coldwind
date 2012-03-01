#ifndef HASHER_CXX
#define HASHER_CXX

#include "hasher.hxx"
#include <glibmm/thread.h>
#include <iostream>
#include "config.h"

Derp::Hasher::Hasher() : m_threadPool(8) {

}

void Derp::Hasher::hash_async(const Derp::Request& request) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Hasher::hash), request), false);
}

void Derp::Hasher::hash(const Derp::Request& request) {
  Glib::RefPtr<Gio::File> base = request.getHashDirectory();
  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory), base));

  auto enumerator = base->enumerate_children("standard::type,standard::name");
  for ( auto info = enumerator->next_file(); info; info = enumerator->next_file()) {
      if ( info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY ) {
	m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory),
				      base->get_child(info->get_name())));
      }
  }

  enumerator->close();

  auto board_dir = base->get_child(Glib::filename_from_utf8(request.getBoard()));
  if (board_dir->query_exists()) {
    auto board_enumerator = board_dir->enumerate_children("standard::type,standard::name");
    for ( auto info = board_enumerator->next_file(); info; info = board_enumerator->next_file()) {
      if ( info->get_file_type() == Gio::FileType::FILE_TYPE_DIRECTORY ) {
	m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory),
				      board_dir->get_child(info->get_name())));
      }
    }
    board_enumerator->close();
  }

  while( m_threadPool.unprocessed() > 0 ) {
    Glib::Thread::yield();
  }
  signal_hashing_finished();
}

void Derp::Hasher::hash_directory(const Glib::RefPtr<Gio::File>& dir) {
  Glib::RefPtr<Gio::FileEnumerator> enumerator = dir->enumerate_children("standard::type,standard::name,standard::size");
  for(auto info = enumerator->next_file(); info; info = enumerator->next_file()) {
    if ( info->get_file_type() != Gio::FileType::FILE_TYPE_REGULAR )
      continue;
    if ( info->get_size() > MAXIMUM_FILESIZE )
      continue;
    if ( info->get_name().find(COLDWIND_PARTIAL_FILENAME_SUFFIX) != std::string::npos )
      continue;

    auto file = dir->get_child(info->get_name());
    if (!is_hashed(file->get_path())) {
      bool tryAgain = true;
      while (tryAgain) {
	tryAgain = false;
	try {
	  m_threadPool.push( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_file), file) );
	} catch ( Glib::ThreadError e ) {
	  switch ( e.code() ) {
	  case Glib::ThreadError::Code::AGAIN:
	    std::cerr << "Error: Couldn't spin up thread for hasher: " << e.what() << std::endl;
	    tryAgain = true;
	    Glib::Thread::yield();
	    break;
	  default:
	    std::cerr << "Error: Unknown threading error while hashing: " << e.what() << std::endl;
	    tryAgain = false;
	    break;
	  }
	}
      }
    }
  }
  enumerator->close();
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
