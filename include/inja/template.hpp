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
  std::string contents;

 public:
  Template() {}
  Template(const Template& oth): bytecodes(oth.bytecodes), contents(oth.contents) {}
  Template(Template&& oth): bytecodes(std::move(oth.bytecodes)), contents(std::move(oth.contents)) {}

  Template& operator=(const Template& oth) {
    bytecodes = oth.bytecodes;
    contents = oth.contents;
    return *this;
  }

  Template& operator=(Template&& oth) {
    bytecodes = std::move(oth.bytecodes);
    contents = std::move(oth.contents);
    return *this;
  }
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP
