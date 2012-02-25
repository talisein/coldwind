#include "image.hxx"
#include <iostream>
#include <glibmm/convert.h>

Derp::Image::Image(const Glib::ustring& uri, const std::string& md5hex, const Glib::ustring& size, int xDim, int yDim, const Glib::ustring& origFilename, const bool& useOriginalFilename) : 
  m_uri(uri),
  m_md5(md5hex),
  m_size(size),
  m_original_filename(origFilename),
  m_x(xDim),
  m_y(yDim),
  useOriginalFilename_(useOriginalFilename)
{
}

Derp::Image::Image(const std::string& filepath, const std::string& md5hex) :
  m_uri(filepath),
  m_md5(md5hex),
  m_size("0 KB"),
  m_original_filename(""),
  m_x(0),
  m_y(0),
  useOriginalFilename_(false)
{
}

bool Derp::Image::is_bigger(int xDim, int yDim) const {
  return m_x >= xDim && m_y >= yDim;
}

bool Derp::operator==(const Image& lhs, const Image& rhs) {
  return lhs.m_md5.compare(rhs.m_md5) == 0;
}

bool Derp::operator!=(const Image& lhs, const Image& rhs) {
  return lhs.m_md5.compare(rhs.m_md5) != 0;
}

bool Derp::operator<(const Image& lhs, const Image& rhs) {
  return lhs.m_md5.compare(rhs.m_md5) < 0;
}

std::string Derp::Image::getFilename() const {
  std::string string;
  
  try {
    if (useOriginalFilename_) {
      string = Glib::filename_from_utf8(m_original_filename);
    } else { 
      string = Glib::filename_from_utf8(m_uri.substr(m_uri.find_last_of("/")+1));
    }
  } catch (Glib::ConvertError e) {
    std::cerr << "Failed to convert filename to an OS-acceptable charset: " << e.what() << std::endl;
  }

  return string;
}
