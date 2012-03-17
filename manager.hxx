#ifndef MANAGER_HXX
#define MANAGER_HXX
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include "parser.hxx"
#include "hasher.hxx"
#include "downloader.hxx"
#include "request.hxx"

namespace Derp {
  
	enum Error {
		THREAD_404,
		DUPLICATE_FILE,
		IMAGE_CURL_ERROR,
		THREAD_CURL_ERROR,
		THREAD_PARSE_ERROR
	};

	class Manager {
	public:
		Manager();

		bool download_async(const Derp::Request& data);
		sigc::signal<void, int, const Derp::Request&> signal_all_downloads_finished;
		sigc::signal<void> signal_download_finished;
		sigc::signal<void, int> signal_starting_downloads;
		sigc::signal<void, const Derp::Error&> signal_download_error;

		double getProgress() const { return m_downloader.getProgress(); };
	private:

		void parsing_finished();
		void hashing_finished();
		void try_download();
		void download_finished();
		void download_error();
		void done();
		void thread_404();
		void thread_fetching_error();
		void thread_parsing_error();
		bool is_working;
		bool is_hashing, is_parsing;
		int num_downloading, num_downloaded, num_errors;

		Derp::Parser m_parser;
		Derp::Hasher m_hasher;
		Derp::Downloader m_downloader;

		Derp::Request m_request;

	};
}

#endif
