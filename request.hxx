#ifndef REQUEST_HXX
#define REQUEST_HXX
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <giomm/file.h>

namespace Derp {
	class Image;

	class Request {
	public:
		Request(const Glib::ustring& thread_url,
		        const Glib::RefPtr<Gio::File>& target_directory,
		        const Glib::ustring& thread_directory,
		        const int& minutes,
		        const int& xDim,
		        const int& yDim,
		        const bool& useBoardSubdir,
		        const bool& useThreadSubdir,
		        const bool& useOriginalFilename,
		        const bool& lurk404);
		Request() = default;

		Glib::RefPtr<Gio::File> getDirectory() const;
		Glib::RefPtr<Gio::File> getHashDirectory() const;
		Glib::ustring getBoard() const;
		Glib::ustring getThread() const;
		Glib::ustring getUrl() const;

		bool isExpired() const;
		void decrementMinutes();
		bool useOriginalFilename() const;

	private:
		Glib::ustring thread_url_;
		Glib::RefPtr<Gio::File> target_directory_;
		Glib::ustring thread_directory_;
		int minutes_;
		int xDim_;
		int yDim_;
		bool useBoardSubdir_;
		bool useThreadSubdir_;
		bool useOriginalFilename_;
		bool lurkTo404_;

		friend bool operator<(const Derp::Image& image, 
		                      const Derp::Request& request);
	};
}


#endif
