#ifndef _LIBMATTING_UTILS_H_
#define _LIBMATTING_UTILS_H_

#include <vector>

struct Point2i {
  Point2i(int x, int y) : x(x), y(y) {}
  int x, y;
};

// A user-provided drawing that is either foreground or background
struct Scribble {
  bool background;
  std::vector<Point2i> pixels;
};


template<class T>
bool IsNaN(T t) {
  return t != t;
}

// Copy src[i] to dst[i] if mask[i] == true_val
template<typename T, typename M=uint8_t>
void MaskedCopy(const T* src,
                const M* mask,
                M true_val,
                const int len,
                T* dst) {
  for (int i = 0; i < len; ++i) {
    if (mask[i]) {
      dst[i] = src[i];
    }
  }
}


#endif
