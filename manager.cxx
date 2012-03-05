#include "manager.hxx"

Derp::Manager::Manager() : 
	is_working(false)
{
	m_parser.signal_parsing_finished.connect(sigc::mem_fun(*this, &Derp::Manager::parsing_finished));
	m_parser.signal_thread_404.connect(sigc::mem_fun(*this, &Derp::Manager::thread_404));
	m_hasher.signal_hashing_finished.connect(sigc::mem_fun(*this, &Derp::Manager::hashing_finished));
	m_downloader.signal_download_finished.connect(sigc::mem_fun(*this, &Derp::Manager::download_finished));
	m_downloader.signal_download_error.connect(sigc::mem_fun(*this, &Derp::Manager::download_error));
}

bool Derp::Manager::download_async(const Derp::Request& data) {
	if (!is_working) {
		is_working = true;
		m_request = data;
		is_hashing = is_parsing = true;
		m_parser.parse_async(m_request);
		m_hasher.hash_async(m_request);
		return true;
	} else {
		return false; 
	}
}

void Derp::Manager::parsing_finished() {
	is_parsing = false;
	try_download();
}

void Derp::Manager::hashing_finished() {
	is_hashing = false;
	try_download();
}

void Derp::Manager::thread_404() {
	is_working = false;
	signal_download_error(Derp::Error::THREAD_404);
	// TODO: Need to manage state better
}

void Derp::Manager::try_download() {
	if ( !(is_parsing || is_hashing) ) {
		num_errors = 0;
		num_downloaded = 0;
		num_downloading = m_parser.request_downloads(m_downloader, m_hasher, m_request);
		if ( num_downloading == 0 ) {
			done();
		} else {
			signal_starting_downloads(num_downloading);
		}
	}
}

void Derp::Manager::download_finished() {
	num_downloaded++;
	signal_download_finished();
	if (num_downloading == (num_downloaded + num_errors)) {
		done();
	}
}

void Derp::Manager::download_error() {
	num_errors++;
	signal_download_error(Derp::Error::CURL_ERROR);
	if (num_downloading == (num_downloaded + num_errors)) {
		done();
	}
}

void Derp::Manager::done() {
	is_working = false;
	signal_all_downloads_finished(num_downloaded, m_request);
}
