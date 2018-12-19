#ifndef PANTOR_INJA_TEMPLATE_HPP
#define PANTOR_INJA_TEMPLATE_HPP

#include <string>
#include <vector>

#include <inja/internal.hpp>


namespace inja {

class Template {
  friend class Parser;
  friend class Renderer;

 public:
  Template() {}

  ~Template() {}

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

 private:
  std::vector<Bytecode> bytecodes;
  std::string contents;
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP