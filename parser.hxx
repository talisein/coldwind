#ifndef PARSER_HXX
#define PARSER_HXX

#include <libxml/HTMLparser.h>
#include <curl/curl.h>
#include <list>
#include <map>
#include <glibmm/ustring.h>
#include <glibmm/datetime.h>
#include <glibmm/thread.h>
#include "image.hxx"
#include "downloader.hxx"
#include "hasher.hxx"
#include "request.hxx"

namespace Derp {

  class Parser
  {
  public:
    Parser();
    ~Parser();

    void parse_async(const Derp::Request& request);
    int request_downloads(Derp::Downloader&, const Derp::Hasher&, const Derp::Request&);
    Glib::Dispatcher signal_parsing_finished;
    Glib::Dispatcher signal_thread_404;
    Glib::Dispatcher signal_fetching_error;
    Glib::Dispatcher signal_parsing_error;


  private:
    Parser& operator=(const Parser&) = delete; // Evil func
    Parser(const Parser&) = delete; // Evil func

    Glib::ustring curSourceUrl, curOrigFilename;
    int curxDim, curyDim;
    std::list<Derp::Image> m_images;
    Derp::Request request_;

    CURL* curl;
    std::map<Glib::ustring, Glib::DateTime> m_lastupdate_map;

    htmlSAXHandlerPtr sax;
    xmlParserCtxtPtr ctxt;
    bool parser_error_;

    bool setup_curl(const Glib::ustring& url);

    void parse_thread(const Derp::Request& request);
    void on_start_element(Glib::ustring name, std::map<Glib::ustring, Glib::ustring> attr_map);
    void on_characters(const Glib::ustring&);

    friend void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
    friend void onCharacters(void* user_data, const xmlChar* chars, int len);
    friend void on_xmlError(void* user_data, xmlErrorPtr error);

    friend size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);
  };

  void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
  void onCharacters(void* user_data, const xmlChar* chars, int len);
  void on_xmlError(void* user_data, xmlErrorPtr error);
  size_t parser_write_cb(void *ptr, size_t size, size_t nmemb, void *data);
}




#endif
