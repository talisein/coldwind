#ifndef PARSER_HXX
#define PARSER_HXX

#include <libxml/HTMLparser.h>
#include <list>
#include <map>
#include <glibmm/ustring.h>
#include "image.hxx"
#include "downloader.hxx"
#include "hasher.hxx"

namespace Derp {

  class Parser
  {
  public:
    Parser();
    ~Parser();

    void parse_async(const Glib::ustring& url);
    int request_downloads(Derp::Downloader& downloader, Derp::Hasher* const hasher, const std::string& path, int xDim = -1, int yDim = -1);
    Glib::Dispatcher signal_parsing_finished;


  private:
    Parser& operator=(const Parser&); // Evil func
    Parser(const Parser&); // Evil func

    Glib::ustring curSourceUrl, curOrigFilename;
    int curxDim, curyDim;
    std::list<Derp::Image> m_images;

    htmlSAXHandlerPtr sax;
    void parse_thread(const Glib::ustring& url);
    void on_start_element(Glib::ustring name, std::map<Glib::ustring, Glib::ustring> attr_map);
    void on_characters(const std::string&);

    friend void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
    friend void onCharacters(void* user_data, const xmlChar* chars, int len);
  };

  void startElement(void* user_data, const xmlChar* name, const xmlChar** attrs);
  void onCharacters(void* user_data, const xmlChar* chars, int len);
}




#endif
