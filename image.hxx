#ifndef IMAGE_HXX
#define IMAGE_HXX
#include <glibmm/ustring.h>

namespace Derp {
  class Image {
  public:
    Image(const Glib::ustring& uri, const Glib::ustring& md5hex, const Glib::ustring& size, const Glib::ustring& xDim, const Glib::ustring& yDim);
    Image(const std::string& filepath, const Glib::ustring& md5hex);

    bool is_bigger(int xDim, int yDim);

    Glib::ustring getUrl() { return m_uri; };
  private:
    // URL or file path
    Glib::ustring m_uri;
    Glib::ustring m_md5;
    Glib::ustring m_size;
    int m_x;
    int m_y;

    friend bool operator!=(const Image& lhs, const Image& rhs);
    friend bool operator==(const Image& lhs, const Image& rhs);
    friend bool operator<(const Image& lhs, const Image& rhs);
  };

  bool operator==(const Image& lhs, const Image& rhs);
  bool operator<(const Image& lhs, const Image& rhs);
  bool operator!=(const Image& lhs, const Image& rhs);

}

#endif
