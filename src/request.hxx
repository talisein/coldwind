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
		        const int minutes,
		        const int xDim,
		        const int yDim,
		        const bool useBoardSubdir,
		        const bool useThreadSubdir,
		        const bool useOriginalFilename,
		        const bool lurk404);
		Request() noexcept = default;
        Request(const Request&) noexcept = default;

		Glib::RefPtr<Gio::File> getDirectory() const;
		Glib::RefPtr<Gio::File> getHashDirectory() const;
		Glib::ustring getBoard() const;
		Glib::ustring getThread() const;
		Glib::ustring getUrl() const;
        std::string get_api_url() const;
        guint64 get_thread_id() const;
        std::size_t get_request_id() const;

		bool isExpired() const;
		void decrementMinutes(int mins = 1);
		void mark404();
		bool useOriginalFilename() const;

        int get_min_width() const { return xDim_; };
        int get_min_height() const { return yDim_; };
	private:
        std::size_t request_id;
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
		bool is404_;

		friend bool operator<(const Derp::Image& image,
		                      const Derp::Request& request);
	};
}


#endif
