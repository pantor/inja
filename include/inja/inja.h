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
#include <vector>
#include <sstream>

#include "wpi/ArrayRef.h"
#include "wpi/SmallString.h"
#include "wpi/StringRef.h"

#include <nlohmann/json.hpp>


namespace inja {


using namespace nlohmann;
using namespace wpi;

class Bytecode;
class Parser;
class Renderer;

using CallbackFunction = std::function<json(ArrayRef<const json*> args, const json& data)>;

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

  void SetLoadFile(std::function<std::string(StringRef)> loadFile);

  Template Parse(StringRef input);

  std::string Render(StringRef input, const json& data);

  std::string Render(const Template& tmpl, const json& data);

  std::stringstream& RenderTo(std::stringstream& os, const Template& tmpl, const json& data);

  void AddCallback(StringRef name, unsigned int numArgs, const CallbackFunction& callback);

  void IncludeTemplate(const std::string& name, const Template& tmpl);

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

/*!
@brief render with default settings
*/
inline std::string Render(StringRef input, const json& data) {
  return Environment().Render(input, data);
}

}  // namespace inja

#endif  // WPIUTIL_WPI_INJA_H_
