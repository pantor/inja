// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TEMPLATE_HPP_
#define INCLUDE_INJA_TEMPLATE_HPP_

#include <map>
#include <string>
#include <vector>

#include "node.hpp"

namespace inja {

/*!
 * \brief The main inja Template.
 */
struct Template {
  std::vector<Node> nodes;
  std::string content;

  /// Return number of variables (total number, not distinct ones) in the template
  int count_variables() {
    int result {0};
    for (auto& n : nodes) {
      if (n.flags == Node::Flag::ValueLookupDot || n.flags == Node::Flag::ValueLookupPointer) {
        result += 1;
      }
    } 
    return result;
  }
};

using TemplateStorage = std::map<std::string, Template>;

} // namespace inja

#endif // INCLUDE_INJA_TEMPLATE_HPP_
