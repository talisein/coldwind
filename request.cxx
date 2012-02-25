#include "request.hxx"
#include <iostream>

Derp::Request::Request(const Glib::ustring& thread_url,
		       const Glib::RefPtr<Gio::File>& target_directory,
		       const int& minutes,
		       const int& xDim,
		       const int& yDim,
		       const bool& useBoardSubdir,
		       const bool& useThreadSubdir,
		       const bool& useOriginalFilename) :
  thread_url_(thread_url),
  target_directory_(target_directory),
  minutes_(minutes),
  xDim_(xDim),
  yDim_(yDim),
  useBoardSubdir_(useBoardSubdir),
  useThreadSubdir_(useThreadSubdir),
  useOriginalFilename_(useOriginalFilename)
{
}

Derp::Request::Request() {
}

void Derp::Request::decrementMinutes() {
  minutes_--;
}

bool Derp::Request::isExpired() const {
  return minutes_ <= 0;
}

Glib::RefPtr<Gio::File> Derp::Request::getHashDirectory() const {
  return target_directory_;
}

Glib::RefPtr<Gio::File> Derp::Request::getDirectory() const {
  // TODO: 
  Glib::RefPtr<Gio::File> dir = target_directory_;

  if ( useBoardSubdir_ ) {
    dir = dir->get_child(Glib::filename_from_utf8(getBoard()));
  }

  if ( useThreadSubdir_ ) {
    dir = dir->get_child(Glib::filename_from_utf8(getThread()));
  }

  return dir;
}

Glib::ustring Derp::Request::getUrl() const {
  return thread_url_;
}

bool Derp::Request::useOriginalFilename() const {
  return useOriginalFilename_;
}



Glib::ustring Derp::Request::getBoard() const {
  Glib::ustring string(thread_url_.substr(0, thread_url_.find_last_of("/res/") - 4));
  string = string.substr(string.find_last_of("/") + 1);
  return string;
}

Glib::ustring Derp::Request::getThread() const {
  return thread_url_.substr(thread_url_.find_last_of("/") + 1);
}
