#include "parser.hxx"
#include <curl/curl.h>
#include <iostream>
#include <cstdio>
#include <glib.h>
#include <iomanip>

static void m_startElement(void* user_data, const xmlChar* name, const xmlChar** attrs) {
  std::map<Glib::ustring, Glib::ustring> attr_map;

  int i = 0;
  while (attrs != NULL && attrs[i] != NULL && attrs[i+1] != NULL) {
    attr_map.insert({reinterpret_cast<const char*>(attrs[i]), reinterpret_cast<const char*>(attrs[i+1])});
    i += 2;
  }
  
  Derp::Parser* parser = (Derp::Parser*) user_data;
  parser->on_start_element( reinterpret_cast<const char*>(name), attr_map );
}

Derp::Parser::Parser() {
  sax = (htmlSAXHandlerPtr) calloc(1, sizeof(htmlSAXHandler));
  sax->startElement = &m_startElement;
}

Derp::Parser::~Parser() {
  free(sax);
}

void Derp::Parser::parse_async(const Glib::ustring& url) {
  Glib::Thread::create( sigc::bind(sigc::mem_fun(this, &Parser::parseThread), url), false);
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
    attr_map.insert({"md5hex", st.str()});
    images_map.insert({curSourceUrl, attr_map});
  }
}

void Derp::Parser::parseThread(const Glib::ustring& url) {
  htmlSAXParseFile(url.c_str(), NULL, sax, this);
  signal_parsing_finished();
}




