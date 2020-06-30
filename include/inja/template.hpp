// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TEMPLATE_HPP_
#define INCLUDE_INJA_TEMPLATE_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "node.hpp"

namespace inja {

/*!
 * \brief The main inja Template.
 */
struct Template {
  BlockNode root;
  std::vector<Node> nodes;
  std::string content;

  explicit Template() { }
  explicit Template(const std::string& content): content(content) { }

  /// Return number of variables (total number, not distinct ones) in the template
  int count_variables() {
    return std::count_if(nodes.cbegin(), nodes.cend(), [](const inja::Node &node) {
      return (node.flags == Node::Flag::ValueLookupDot || node.flags == Node::Flag::ValueLookupPointer);
    });
  }
};

using TemplateStorage = std::map<std::string, Template>;

} // namespace inja

#endif // INCLUDE_INJA_TEMPLATE_HPP_
