#ifndef IMAGE_HXX
#define IMAGE_HXX
#include <glibmm/ustring.h>


namespace Derp {
  class Image {
  public:
    Image(const Glib::ustring& uri, const std::string& md5hex, const Glib::ustring& size, int xDim, int yDim, const Glib::ustring& origFilename);
    Image(const std::string& filepath, const std::string& md5hex);

    bool is_bigger(int xDim, int yDim) const;

    Glib::ustring getUrl() const { return m_uri; };
  private:
    // URL or file path
    Glib::ustring m_uri;
    std::string m_md5;
    Glib::ustring m_size;
    int m_x;
    int m_y;
    Glib::ustring m_original_filename;

    friend bool operator!=(const Image& lhs, const Image& rhs);
    friend bool operator==(const Image& lhs, const Image& rhs);
    friend bool operator<(const Image& lhs, const Image& rhs);
  };

  bool operator==(const Image& lhs, const Image& rhs);
  bool operator<(const Image& lhs, const Image& rhs);
  bool operator!=(const Image& lhs, const Image& rhs);

}

#endif
