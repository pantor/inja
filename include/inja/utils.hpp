#ifndef PANTOR_INJA_UTILS_HPP
#define PANTOR_INJA_UTILS_HPP

#include <stdexcept>
#include <string_view>


namespace inja {

inline void inja_throw(const std::string& type, const std::string& message) {
  throw std::runtime_error("[inja.exception." + type + "] " + message);
}

inline std::string_view string_view_slice(std::string_view view, size_t start, size_t end) {
  start = std::min(start, view.size());
  end = std::min(std::max(start, end), view.size());
  return view.substr(start, end - start); // StringRef(Data + Start, End - Start);
}

inline std::pair<std::string_view, std::string_view> string_view_split(std::string_view view, char Separator) {
  size_t idx = view.find(Separator);
  if (idx == std::string_view::npos) {
    return std::make_pair(view, std::string_view());
  }
  return std::make_pair(string_view_slice(view, 0, idx), string_view_slice(view, idx + 1, std::string_view::npos));
}

inline bool string_view_starts_with(std::string_view view, std::string_view prefix) {
  return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
}

}  // namespace inja

#endif  // PANTOR_INJA_UTILS_HPP
