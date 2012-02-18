#include "image.hxx"
#include <string>

Derp::Image::Image(const Glib::ustring& uri, const Glib::ustring& md5hex, const Glib::ustring& size, const Glib::ustring& xDim, const Glib::ustring& yDim) : 
  m_uri(uri),
  m_md5(md5hex),
  m_size(size)
{
  std::stringstream st1, st2;
  st1 << xDim;
  st2 << yDim;
  st1 >> m_x;
  st2 >> m_y;
}

Derp::Image::Image(const std::string& filepath, const Glib::ustring& md5hex) :
  m_uri(filepath),
  m_md5(md5hex)
{
}

bool Derp::Image::is_bigger(int xDim, int yDim) {
  return m_x > xDim && m_y > yDim;
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
