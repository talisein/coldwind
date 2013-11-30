#include "request.hxx"
#include <atomic>
#include <iostream>
#include <glibmm/convert.h>

Derp::Request::Request(const Glib::ustring& thread_url,
		       const Glib::RefPtr<Gio::File>& target_directory,
		       const Glib::ustring& thread_directory,
		       const int minutes,
		       const int xDim,
		       const int yDim,
		       const bool useBoardSubdir,
		       const bool useThreadSubdir,
		       const bool useOriginalFilename,
		       const bool lurkTo404) :
  thread_url_(thread_url),
  target_directory_(target_directory),
  thread_directory_(thread_directory),
  minutes_(minutes),
  xDim_(xDim),
  yDim_(yDim),
  useBoardSubdir_(useBoardSubdir),
  useThreadSubdir_(useThreadSubdir),
  useOriginalFilename_(useOriginalFilename),
  lurkTo404_(lurkTo404),
  is404_(false)
{
    static std::atomic<std::size_t> global_counter(0);
    ++global_counter;

    request_id = global_counter;
}

void Derp::Request::decrementMinutes(int mins) {
  minutes_ -= mins;
}

bool Derp::Request::isExpired() const {
	if (is404_)
		return true;

	if ( !lurkTo404_) {
		return minutes_ <= 0;
	}

	return false;
}

void Derp::Request::mark404() {
	is404_ = true;
}

Glib::RefPtr<Gio::File> Derp::Request::getHashDirectory() const {
  return target_directory_;
}

Glib::RefPtr<Gio::File> Derp::Request::getDirectory() const {
  // TODO: 
  Glib::RefPtr<Gio::File> dir = target_directory_;

  if ( useBoardSubdir_ ) {
    dir = dir->get_child_for_display_name(getBoard());
  }

  if ( useThreadSubdir_ ) {
    dir = dir->get_child_for_display_name(thread_directory_);
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
  Glib::ustring string(thread_url_.substr(0, thread_url_.find_last_of('/') - 4));
  string = string.substr(string.find_last_of("/") + 1);
  return string;
}

Glib::ustring Derp::Request::getThread() const {
  return thread_url_.substr(thread_url_.find_last_of("/") + 1);
}

namespace Derp {
    std::size_t
    Request::get_request_id() const
    {
        return request_id;
    }

    std::string
    Request::get_api_url() const
    {
        std::stringstream ss;
        ss << "http://api.4chan.org/";
        ss << getBoard();
        ss << "/res/";
        auto number = getThread();;
        ss << number.substr(0, number.find_first_not_of("0123456789"));
        ss << ".json";
        return ss.str();
    }

    guint64
    Request::get_thread_id() const
    {
        std::stringstream ss;
        guint64 id;
        ss << getThread();
        ss >> id;
        return id;
    }
}
