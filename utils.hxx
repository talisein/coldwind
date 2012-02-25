#include "request.hxx"
#include "image.hxx"

namespace Derp {
  bool operator<(const Derp::Image& image, const Derp::Request& request)
  { return image.m_x < request.xDim_ || image.m_y < request.yDim_; }
}
