#ifndef INCLUDE_INJA_JSON_FWD_HPP_
#define INCLUDE_INJA_JSON_FWD_HPP_

#include <nlohmann/json_fwd.hpp>

namespace inja {
#ifndef INJA_DATA_TYPE
using json = nlohmann::json;
#else
using json = INJA_DATA_TYPE;
#endif
} // namespace inja

#endif // INCLUDE_INJA_JSON_FWD_HPP_
