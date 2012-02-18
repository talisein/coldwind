#ifndef PARSER_HXX
#define PARSER_HXX
#include <glibmm.h>
#include <giomm.h>
#include <libxml/HTMLparser.h>
#include <set>

namespace Derp {

  class Parser
  {
  public:
    explicit Parser();
    ~Parser();
    void parse_async(const Glib::ustring& url);
    void on_start_element(Glib::ustring name, std::map<Glib::ustring, Glib::ustring> attr_map);

    Glib::Dispatcher signal_parsing_finished;

    /**
       This single function (below) is the worst part of this program
       from a design perspective. We need it so that we can compare
       against the hasher's results and produce a list for the
       downloader. The real-world performance impact is pretty
       negligable.

       Ideally we should do something like
       Downloader.download_async(filter(Hasher.is_downloaded_functor,
                                        Parser.list))
					
       So far, my experiments at using <functional> have failed
       though. In the future, I think I should look at refactoring
       images_map by creating an Image class (that has an inner
       attribute map). That would give a simpler list here, and the
       Image class can define useful comparison functions.
    **/
    std::map<Glib::ustring, std::map<Glib::ustring, Glib::ustring>> get_image_sources() const { return images_map; };    
  private:

    Parser& operator=(const Parser&); // Evil func
    Parser(const Parser&); // Evil func

    void parseThread(const Glib::ustring& url);

    htmlSAXHandlerPtr sax;
    Glib::ustring curSourceUrl;
    std::map<Glib::ustring, std::map<Glib::ustring, Glib::ustring>> images_map;
    std::set<Glib::ustring> downloadedImages;
  };
}




#endif
