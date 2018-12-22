#ifndef PANTOR_INJA_TEMPLATE_HPP
#define PANTOR_INJA_TEMPLATE_HPP

#include <string>
#include <vector>

#include "bytecode.hpp"


namespace inja {

class Template {
  friend class Parser;
  friend class Renderer;

  std::vector<Bytecode> bytecodes;
  std::string content;

 public:
  Template() {}
  Template(const Template& oth): bytecodes(oth.bytecodes), content(oth.content) {}
  Template(Template&& oth): bytecodes(std::move(oth.bytecodes)), content(std::move(oth.content)) {}

  Template& operator=(const Template& oth) {
    bytecodes = oth.bytecodes;
    content = oth.content;
    return *this;
  }

  Template& operator=(Template&& oth) {
    bytecodes = std::move(oth.bytecodes);
    content = std::move(oth.content);
    return *this;
  }
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP
