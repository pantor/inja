#ifndef PANTOR_INJA_UTILS_HPP
#define PANTOR_INJA_UTILS_HPP

#include <string>


namespace inja {
	/*!
	@brief render with default settings
	*/
	inline std::string render(const std::string& input, const json& data) {
		return Environment().render(input, data);
	}
}

#endif // PANTOR_INJA_UTILS_HPP
