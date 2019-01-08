#ifndef PANTOR_INJA_UTILS_HPP
#define PANTOR_INJA_UTILS_HPP

#include <stdexcept>
#include <string_view>


namespace inja {

	inline void inja_throw(const std::string& type, const std::string& message) {
		throw std::runtime_error("[inja.exception." + type + "] " + message);
	}

	namespace string_view {
		inline std::string_view slice(std::string_view view, size_t start, size_t end) {
			start = std::min(start, view.size());
			end = std::min(std::max(start, end), view.size());
			return view.substr(start, end - start); // StringRef(Data + Start, End - Start);
		}

		inline std::pair<std::string_view, std::string_view> split(std::string_view view, char Separator) {
			size_t idx = view.find(Separator);
			if (idx == std::string_view::npos) {
				return std::make_pair(view, std::string_view());
			}
			return std::make_pair(slice(view, 0, idx), slice(view, idx + 1, std::string_view::npos));
		}

		inline bool starts_with(std::string_view view, std::string_view prefix) {
			return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
		}
	}  // namespace string

}  // namespace inja

#endif  // PANTOR_INJA_UTILS_HPP
