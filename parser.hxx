#ifndef PARSER_HXX
#define PARSER_HXX

#include <libxml/HTMLparser.h>
#include <list>
#include <map>
#include <glibmm/ustring.h>
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
    Glib::Dispatcher signal_parsing_error;


  private:
    Parser& operator=(const Parser&); // Evil func
    Parser(const Parser&); // Evil func

    Glib::ustring curSourceUrl, curOrigFilename;
    int curxDim, curyDim;
    std::list<Derp::Image> m_images;
    Derp::Request request_;

    htmlSAXHandlerPtr sax;
    xmlParserCtxtPtr ctxt;
    bool parser_error_;

    void parse_thread(const Derp::Request& request);
    void on_start_element(Glib::ustring name, std::map<Glib::ustring, Glib::ustring> attr_map);
    void on_characters(const Glib::ustring&);

    friend void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
    friend void onCharacters(void* user_data, const xmlChar* chars, int len);
    friend void on_xmlError(void* user_data, xmlErrorPtr error);
  };

  void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
  void onCharacters(void* user_data, const xmlChar* chars, int len);
  void on_xmlError(void* user_data, xmlErrorPtr error);
}




#endif
