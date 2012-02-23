#ifndef REQUEST_HXX
#define REQUEST_HXX

namespace Derp {
  struct Request {
    Glib::ustring thread_url;
    Glib::RefPtr<Gio::File> target_directory;
    int minutes;
    int xDim;
    int yDim;
  };
}


#endif
