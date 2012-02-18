#include "hasher.hxx"
#include <glibmm/thread.h>
#include <iostream>

Derp::Hasher::Hasher() : m_threadPool(8) {

}

void Derp::Hasher::hash_async(const Glib::RefPtr<Gio::File>& dir) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(*this, &Hasher::hash_directory), dir), false);
}

void Derp::Hasher::hash_directory(const Glib::RefPtr<Gio::File>& dir) {
  Glib::RefPtr<Gio::FileEnumerator> enumerator = dir->enumerate_children();
  for(auto info = enumerator->next_file(); info != 0; info = enumerator->next_file()) {
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
    if (file->load_contents(contents, length)) {
      Glib::ustring md5hex = Glib::Checksum::compute_checksum(Glib::Checksum::ChecksumType::CHECKSUM_MD5, reinterpret_cast<guchar*>(contents), length);
      insert_hash(md5hex);
      insert_filepath(file->get_path());
      g_free(contents);
    }
}

void Derp::Hasher::insert_hash(const Glib::ustring& hash) {
  Glib::Mutex::Lock lock(hash_mutex);
  m_hash_set.insert(hash);
}

void Derp::Hasher::insert_filepath(const std::string& path) {
  Glib::Mutex::Lock lock(filepath_mutex);
  m_filepath_set.insert(path);
}

bool Derp::Hasher::is_hashed(const std::string& path) {
  Glib::Mutex::Lock lock(filepath_mutex);
  return m_filepath_set.count(path) > 0;
}

bool Derp::Hasher::is_downloaded(const Glib::ustring& hash) {
  Glib::Mutex::Lock lock(hash_mutex);
  return m_hash_set.count(hash) > 0;
}
