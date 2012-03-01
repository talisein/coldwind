#include <iostream>
#include <iomanip>
#include <algorithm>
#include "parser.hxx"
#include "utils.hxx"

Derp::Parser::Parser() : parser_error_(false) {
  sax = (htmlSAXHandlerPtr) calloc(1, sizeof(xmlSAXHandler));
  sax->startElement = &startElement;
  sax->characters = &onCharacters;
  sax->serror = &on_xmlError;
  sax->initialized = XML_SAX2_MAGIC;

  ctxt = htmlCreatePushParserCtxt(sax, this, NULL, 0, NULL, XML_CHAR_ENCODING_UTF8);
  htmlCtxtUseOptions(ctxt, HTML_PARSE_RECOVER | HTML_PARSE_NONET);

  curl = curl_easy_init();
}

Derp::Parser::~Parser() {
  curl_easy_cleanup(curl);
  htmlFreeParserCtxt(ctxt);
  free(sax);
}

void Derp::on_xmlError(void* user_data, xmlErrorPtr error) {
  Derp::Parser* parser = static_cast<Derp::Parser*>(user_data);
  switch (error->code) {
  case XML_ERR_NAME_REQUIRED:
  case XML_ERR_TAG_NAME_MISMATCH:
  case XML_ERR_ENTITYREF_SEMICOL_MISSING:
    // Ignore
    break;
  default:
    if (error->domain == XML_FROM_HTML) {
      std::cerr << "Warning: Got unexpected libxml2 HTML error code " 
		<< error->code << ". This is probably ok." << std::endl;
    } else {
      std::cerr << "Error: Got unexpected libxml2 error code " 
		<< error->code << " from domain " << error->domain << " which means: " 
		<< error->message << std::endl;
      std::cerr << "\tPlease report this error to the developer." << std::endl;
      parser->parser_error_ = true;
    }
  }
}

/* Uses libxml to fetch the 4ch thread from the network. Parsing
   happens in the SAX methods.
 */
void Derp::Parser::parse_thread(const Derp::Request& request) {
  request_ = request;
  parser_error_ = false;
  bool fetchingError = false;
  Glib::DateTime now = Glib::DateTime::create_now_utc();
  
  if (!setup_curl( request_.getUrl() )) {
    std::cerr << "Error: Couldn't setup curl for fetching " << request_.getUrl() << std::endl;
    signal_fetching_error();
    return;
  }

  CURLcode code = curl_easy_perform(curl);

  /**
     Handle curl errors
  **/
  if (code != CURLE_OK) {
    fetchingError = true;
    long responseCode;
    switch (code) {
    case CURLE_HTTP_RETURNED_ERROR:
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
      if (responseCode == 404) {
	signal_thread_404();
      } else {
	std::cerr << "Error: Curl ran into an HTTP " << responseCode << " error.";
	signal_fetching_error();
      }
      break;
    default:
      std::cerr << "Error: Curl couldn't fetch the thread: " << curl_easy_strerror(code) << std::endl;
      signal_fetching_error();
    }
  }
  /**
     Curl errors are handled
  **/

  if (!fetchingError) {
    htmlParseChunk(ctxt, 0, 0, 1);
    if (!parser_error_) {
      m_lastupdate_map.erase(request_.getUrl());
      m_lastupdate_map.insert({request_.getUrl(), now});
      signal_parsing_finished();
    }
  }
}

size_t Derp::parser_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
  Derp::Parser* parser = static_cast<Derp::Parser*>(userdata);
  
  htmlParseChunk(parser->ctxt, static_cast<char*>(ptr), size*nmemb, 0);
  return size*nmemb;
}

static bool is_code_ok(CURLcode code, std::string str) {
  if (code != CURLE_OK) {
    std::cerr << "Error: While setting up curl to fetch the thread in " << str << " : " << curl_easy_strerror(code) << std::endl;
    return false;
  }
  return true;
}

bool Derp::Parser::setup_curl(const Glib::ustring& url) {
  CURLcode code;
  bool ok = true;

  curl_easy_reset(curl);

  code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  ok = ok && is_code_ok(code, "URL");

  code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Derp::parser_write_cb);
  ok = ok && is_code_ok(code, "WRITEFUNCTION");

  code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
  ok = ok && is_code_ok(code, "WRITEDATA");

  code = curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  ok = ok && is_code_ok(code, "FAILONERROR");

  if ( m_lastupdate_map.count(url) > 0 ) {
    code = curl_easy_setopt(curl, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
    ok = ok && is_code_ok(code, "TIMECONDITION");

    code = curl_easy_setopt(curl, CURLOPT_TIMEVALUE, m_lastupdate_map.find(url)->second.to_unix());
    ok = ok && is_code_ok(code, "TIMEVALUE");
  }

  return ok;
}

void Derp::startElement(void* user_data, const xmlChar* name, const xmlChar** attrs) {
  Derp::Parser* parser = (Derp::Parser*) user_data;
  std::map<Glib::ustring, Glib::ustring> attr_map;

  int i = 0;
  while (attrs != NULL && attrs[i] != NULL && attrs[i+1] != NULL) {
    attr_map.insert({reinterpret_cast<const char*>(attrs[i]), reinterpret_cast<const char*>(attrs[i+1])});
    i += 2;
  }
  
  try {
    Glib::ustring str( reinterpret_cast<const char*>(name) );
    parser->on_start_element( str, attr_map );
  } catch (std::exception e) {
    std::cerr << "Error startElement casting to string: " << e.what() << std::endl;
  }
}

void Derp::onCharacters(void* user_data, const xmlChar* chars, int) {
  Derp::Parser* parser = static_cast<Derp::Parser*>(user_data);
  Glib::ustring str;

  try {
    str.assign(reinterpret_cast<const char*>(chars));
  } catch (std::exception e) {
    std::cerr << "Error casting '" << chars << "' to Glib::ustring: " << e.what() << std::endl;
  }

  parser->on_characters(str);
}

void Derp::Parser::parse_async(const Derp::Request& request) {
  m_images.clear();
  Glib::Thread::create( sigc::bind(sigc::mem_fun(this, &Parser::parse_thread), request), false);
}

void Derp::Parser::on_characters(const Glib::ustring& str) {
  if ( curSourceUrl.size() > 0) {
    size_t start = str.find(", ");
    if (start != std::string::npos) {
      start += 2;
      size_t middle = str.find("x", start);
      if (middle != std::string::npos) {
	std::stringstream st1, st2;
	st1 << str.substr(start, middle-start);
	st1 >> curxDim;
	middle += 1;
	size_t end = str.find(",", middle);
	st2 << str.substr(middle, end-middle);
	st2 >> curyDim;
      }
    }
  }
}

void Derp::Parser::on_start_element(Glib::ustring name, std::map<Glib::ustring, Glib::ustring> attr_map) {
  // If this element contains an image source url, store it
  if (name.length() == 1 && name.find("a") != Glib::ustring::npos ) {
    if( attr_map.count("href") != 0 ) {
      auto it = attr_map.find("href");
      if (it->second.find("/src/") != Glib::ustring::npos) {
	curSourceUrl = it->second;
      }
    }
  }

  if (name.find("span") != Glib::ustring::npos && attr_map.count("title") > 0) {
    curOrigFilename = attr_map.find("title")->second;
  }

  // If this element is image metadata, insert it into our set 
  //
  // Note: 
  // 4chan gives us the md5 in Base64, whereas Glib computes it as a
  // hex string.  So, we convert the base64 to hex here and insert it
  // into the attribute map
  if (attr_map.count("md5") != 0) {
    Glib::ustring md5_base64(attr_map.find("md5")->second);
    gsize len = 0;
    guchar* md5_binary = g_base64_decode(md5_base64.c_str(), &len);
    std::stringstream st;
    st.flags( std::ios::hex );
    st.fill('0');
    for(gsize i = 0; i < len; i++) {
      st << std::setw(2) << static_cast<int>(md5_binary[i]);
    }
    g_free(md5_binary);
    m_images.push_back({curSourceUrl, st.str(), attr_map.find("alt")->second, curxDim, curyDim, curOrigFilename, request_.useOriginalFilename()});
    curSourceUrl = "";

  }
}

int Derp::Parser::request_downloads(Derp::Downloader& downloader, const Derp::Hasher& hasher, const Derp::Request& request) { 

  m_images.remove_if([&hasher](Derp::Image image) { return hasher.is_downloaded(image); });

  m_images.remove_if([&request](Derp::Image image) { return image < request; });

  downloader.download_async(m_images, request);
  return m_images.size();
}


