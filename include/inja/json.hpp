#ifndef INCLUDE_INJA_JSON_HPP_
#define INCLUDE_INJA_JSON_HPP_

#include <nlohmann/json.hpp>

namespace inja {
#ifndef INJA_DATA_TYPE
using json = nlohmann::json;
#else
using json = INJA_DATA_TYPE;
#endif
} // namespace inja

#endif // INCLUDE_INJA_JSON_HPP_
