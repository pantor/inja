#ifndef INCLUDE_INJA_JSON_HPP_
#define INCLUDE_INJA_JSON_HPP_

#include <string>

#define INJA_JSONCONS
#ifdef INJA_JSONCONS
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpointer/jsonpointer.hpp>
#else
#include <nlohmann/json.hpp>
#endif // def INJA_JSONCONS

namespace inja {
#ifndef INJA_DATA_TYPE
#ifdef INJA_JSONCONS
using json = jsoncons::json;
using json_pointer = jsoncons::jsonpointer::json_pointer;
namespace json_ {

template <typename T>
inline T as(json& j) {
    return j.as<T>();
}
template <typename T>
inline const T as(const json& j) {
    return j.as<T>();
}

inline bool is_null(const json& j) noexcept {
	return j.is_null();
}
inline bool is_string(const json& j) noexcept {
	return j.is_string();
}
inline bool is_bignum(const json& j) {
	return j.is_bignum();
}
inline bool is_bool(const json& j) noexcept {
    return j.is_bool();
}
inline bool is_object(const json& j) noexcept {
	return j.is_object();
}
inline bool is_array(const json& j) noexcept {
	return j.is_array();
}
inline bool is_int64(const json& j) noexcept {
	return j.is_int64();
}
inline bool is_uint64(const json& j) noexcept {
	return j.is_uint64();
}
inline bool is_half(const json& j) noexcept {
	return j.is_half();
}
inline bool is_float(const json& j) noexcept {
	return j.is_double();
}
inline bool is_number(const json& j) noexcept {
	return j.is_number();
}
inline bool is_empty(const json& j) noexcept {
	return j.empty();
}

inline bool contains(const json& j, const json_pointer& p) {
	return jsoncons::jsonpointer::contains(j, p);
}
inline bool contains(const json& j, const std::string_view& p) {
	return jsoncons::jsonpointer::contains(j, p);
}

inline bool has(const json& j, const std::string_view& name) {
	return j.contains(name);
}

inline json& get(json& j, const json_pointer& p) {
	return jsoncons::jsonpointer::get(j, p);
}
inline const json& get(const json& j, const json_pointer& p) {
	return jsoncons::jsonpointer::get(j, p);
}

inline json::array_range_type array_range(json& j) noexcept {
	assert(is_array(j));
	return j.array_range();
}
inline json::const_array_range_type array_range(const json& j) noexcept {
	assert(is_array(j));
	return j.array_range();
}

inline auto object_range(json& j) noexcept {
	assert(is_object(j));
	return j.object_range();
}
inline const auto object_range(const json& j) noexcept {
	assert(is_object(j));
	return j.object_range();
}

template <typename T>
inline void set_value(json& j, const json_pointer& ptr, T&& val) {
    jsoncons::jsonpointer::replace(j, ptr, val, true);
}
template <typename T>
inline void set_value(json& j, const std::string_view& ptr, T&& val) {
    jsoncons::jsonpointer::replace(j, json_pointer(ptr), val, true);
}

} // namespace json_
#else
using json = nlohmann::json;
using json_pointer = json::json_pointer;

namespace json_ {

template <typename T>
inline T as(json& j) {
    return j.get<T>();
}
template <typename T>
inline T as(const json& j) {
    return j.get<T>();
}
template <>
inline std::string_view as<std::string_view>(const json& j) {
    auto& v = j.get_ref<const json::string_t&>();
    return std::string_view(v);
}

inline bool is_null(const json& j) noexcept {
	return j.is_null();
}
inline bool is_string(const json& j) noexcept {
	return j.is_string();
}
inline bool is_bool(const json& j) noexcept {
    return j.is_boolean();
}
inline bool is_object(const json& j) noexcept {
	return j.is_object();
}
inline bool is_array(const json& j) noexcept {
	return j.is_array();
}
inline bool is_int64(const json& j) noexcept {
	return j.is_number_integer();
}
inline bool is_uint64(const json& j) noexcept {
	return j.is_number_unsigned();
}
inline bool is_number(const json& j) noexcept {
	return j.is_number();
}
inline bool is_float(const json& j) noexcept {
	return j.is_number_float();
}
inline bool is_empty(const json& j) noexcept {
	return j.empty();
}

inline bool contains(const json& j, const json_pointer& p) {
	return j.contains(p);
}
inline bool contains(const json& j, const std::string_view& p) {
	return j.contains(p);
}

inline bool has(const json& j, const std::string_view& name) {
	return j.find(name) != j.end();
}

inline json& get(json& j, const json_pointer& p) {
	return j[p];
}
inline const json& get(const json& j, const json_pointer& p) {
	return j[p];
}

inline json& array_range(json& j) noexcept {
	assert(is_array(j));
	return j;
}
inline const json& array_range(const json& j) noexcept {
	assert(is_array(j));
	return j;
}

inline auto object_range(json& j) noexcept {
	assert(is_object(j));
	return j.items();
}
inline const auto object_range(const json& j) noexcept {
	assert(is_object(j));
	return j.items();
}

template <typename T>
inline void set_value(json& j, const json_pointer& ptr, T&& val) {
    j[ptr] = val;
}
template <typename T>
inline void set_value(json& j, const std::string& ptr, T&& val) {
    j[json_pointer(ptr)] = val;
}
template <typename T>
inline void set_value(json& j, const std::string_view& ptr, T&& val) {
    j[json_pointer(std::string(ptr))] = val;
}

} // namespace json_

#endif // def INJA_JSONCONS
#else
using json = INJA_DATA_TYPE;
#endif
} // namespace inja

#endif // INCLUDE_INJA_JSON_HPP_
