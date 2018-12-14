#ifndef PANTOR_INJA_ERROR_HPP
#define PANTOR_INJA_ERROR_HPP

#include <string>


namespace inja {

/*!
@brief throw an error with a given message
*/
inline void inja_throw(const std::string& type, const std::string& message) {
	throw std::runtime_error("[inja.exception." + type + "] " + message);
}

}

#endif // PANTOR_INJA_ERROR_HPP
