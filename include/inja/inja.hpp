/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#ifndef WPIUTIL_WPI_INJA_H_
#define WPIUTIL_WPI_INJA_H_

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <sstream>


#include <nlohmann/json.hpp>


namespace inja {


using namespace nlohmann;


class Bytecode;
class Parser;
class Renderer;


using CallbackFunction = std::function<json(std::vector<const json*>& args, const json& data)>;


inline std::string_view string_view_slice(std::string_view view, size_t Start, size_t End) {
  Start = std::min(Start, view.size());
  End = std::min(std::max(Start, End), view.size());
  return view.substr(Start, End - Start); // StringRef(Data + Start, End - Start);
}

inline std::pair<std::string_view, std::string_view> string_view_split(std::string_view view, char Separator) {
  size_t Idx = view.find(Separator);
  if (Idx == std::string_view::npos) {
    return std::make_pair(view, std::string_view());
    return std::make_pair(string_view_slice(view, 0, Idx), string_view_slice(view, Idx+1, std::string_view::npos));
  }
}

inline bool string_view_starts_with(std::string_view view, std::string_view prefix) {
  return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
}



class Template {
  friend class Parser;
  friend class Renderer;

 public:
  // These must be out of line because Bytecode is forward declared
  Template();
  ~Template();
  Template(const Template&);
  Template(Template&& oth);
  Template& operator=(const Template&);
  Template& operator=(Template&&);

 private:
  std::vector<Bytecode> bytecodes;
  std::string contents;
};

enum class ElementNotation {
  Dot,
  Pointer
};

class Environment {
 public:
  Environment();
  explicit Environment(const std::string& path);
  ~Environment();

  void SetStatement(const std::string& open, const std::string& close);

  void SetLineStatement(const std::string& open);

  void SetExpression(const std::string& open, const std::string& close);

  void SetComment(const std::string& open, const std::string& close);

  void SetElementNotation(ElementNotation notation);

  void SetLoadFile(std::function<std::string(std::string_view)> loadFile);

  Template Parse(std::string_view input);

  std::string Render(std::string_view input, const json& data);

  std::string Render(const Template& tmpl, const json& data);

  std::stringstream& RenderTo(std::stringstream& os, const Template& tmpl, const json& data);

  void AddCallback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback);

  void IncludeTemplate(const std::string& name, const Template& tmpl);

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

/*!
@brief render with default settings
*/
inline std::string Render(const std::string& input, const json& data) {
  return Environment().Render(input, data);
}

}  // namespace inja

#endif  // WPIUTIL_WPI_INJA_H_
