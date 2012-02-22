#include <glib.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include "parser.hxx"

Derp::Parser::Parser() {
  sax = (htmlSAXHandlerPtr) calloc(1, sizeof(htmlSAXHandler));
  sax->startElement = &startElement;
  sax->characters = &onCharacters;
}

Derp::Parser::~Parser() {
  free(sax);
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

void Derp::onCharacters(void* user_data, const xmlChar* chars, int len) {
  Derp::Parser* parser = static_cast<Derp::Parser*>(user_data);
  
  Glib::ustring str;
  try {
    str.assign(reinterpret_cast<const char*>(chars));
  } catch (std::exception e) {
    std::cerr << "Error onCharacters casting to string: " << e.what() << std::endl;
    std::cerr << "The characters were: " << chars << std::endl;
  }
  parser->on_characters(str);
}

void Derp::Parser::parse_async(const Glib::ustring& url) {
  m_images.clear();
  Glib::Thread::create( sigc::bind(sigc::mem_fun(this, &Parser::parse_thread), url), false);
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
    m_images.push_back({curSourceUrl, st.str(), attr_map.find("alt")->second, curxDim, curyDim, curOrigFilename});
    curSourceUrl = "";

  }
}

/* Uses libxml to fetch the 4ch thread from the network. Parsing
   happens in the SAX methods.
 */
void Derp::Parser::parse_thread(const Glib::ustring& url) {
  htmlSAXParseFile(url.c_str(), NULL, sax, this);
  signal_parsing_finished();
}

int Derp::Parser::request_downloads(Derp::Downloader& downloader, Derp::Hasher* const hasher_ptr, const Glib::RefPtr<Gio::File> p_dir, int xDim, int yDim) {
  std::list<Derp::Image> imgs;
    
  m_images.remove_if([hasher_ptr](Derp::Image img) { return hasher_ptr->is_downloaded(img); });

  m_images.remove_if([xDim, yDim](Derp::Image img) { return !img.is_bigger(xDim, yDim); });

  int count = m_images.size();
  downloader.download_imgs_multi(m_images, p_dir);
  return count;
}


