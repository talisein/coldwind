#ifndef IMAGE_HXX
#define IMAGE_HXX
#include <glibmm/ustring.h>

namespace Derp {
  class Request;

  class Image {
  public:
    Image(const Glib::ustring& uri, const std::string& md5hex, const Glib::ustring& size, int xDim, int yDim, const Glib::ustring& origFilename, const bool& useOriginalFilename);
    Image(const std::string& filepath, const std::string& md5hex);

    bool is_bigger(int xDim, int yDim) const;

    Glib::ustring getUrl() const { return m_uri; }
    std::string getFilename() const;

  private:
    // URL or file path
    Glib::ustring m_uri;
    std::string m_md5;
    Glib::ustring m_size;
    Glib::ustring m_original_filename;
    int m_x;
    int m_y;
    bool useOriginalFilename_;

    friend bool operator!=(const Image& lhs, const Image& rhs);
    friend bool operator==(const Image& lhs, const Image& rhs);
    friend bool operator<(const Image& lhs, const Image& rhs);
    friend bool operator<(const Derp::Image& image, const Derp::Request& request);
  };

  bool operator==(const Image& lhs, const Image& rhs);
  bool operator<(const Image& lhs, const Image& rhs);
  bool operator!=(const Image& lhs, const Image& rhs);
}

#endif
