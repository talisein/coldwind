#include "image.hxx"

Derp::Image::Image(const Glib::ustring& uri, const std::string& md5hex, const Glib::ustring& size, int xDim, int yDim, const Glib::ustring& origFilename) : 
  m_uri(uri),
  m_md5(md5hex),
  m_size(size),
  m_x(xDim),
  m_y(yDim),
  m_original_filename(origFilename)
{
}

Derp::Image::Image(const std::string& filepath, const std::string& md5hex) :
  m_uri(filepath),
  m_md5(md5hex)
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
