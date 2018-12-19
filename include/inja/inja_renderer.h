/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#ifndef WPIUTIL_INJA_RENDERER_H_
#define WPIUTIL_INJA_RENDERER_H_

#include <algorithm>
#include <numeric>
#include <sstream>
#include <vector>

#include "wpi/ArrayRef.h"
#include "wpi/SmallString.h"
#include "wpi/SmallVector.h"
#include "wpi/StringRef.h"

#include <nlohmann/json.hpp>

#include "inja/inja.h"
#include "inja/inja_internal.h"


namespace inja {

using namespace wpi;

StringRef ConvertDotToJsonPointer(StringRef dot, SmallVectorImpl<char>& out);

class Renderer {
 public:
  Renderer(const TemplateStorage& includedTemplates, const FunctionStorage& callbacks)
      : m_includedTemplates(includedTemplates), m_callbacks(callbacks) {}

  void RenderTo(std::stringstream& os, const Template& tmpl, const json& data);

 private:
  ArrayRef<const json*> GetArgs(const Bytecode& bc);
  void PopArgs(const Bytecode& bc);
  const json* GetImm(const Bytecode& bc);
  void UpdateLoopData();

  const TemplateStorage& m_includedTemplates;
  const FunctionStorage& m_callbacks;

  SmallVector<json, 16> m_stack;

  struct LoopLevel {
    StringRef keyName;              // variable name for keys
    StringRef valueName;            // variable name for values
    json data;                      // data with loop info added

    json values;                    // values to iterate over

    // loop over list
    json::iterator it;              // iterator over values
    size_t index;                   // current list index
    size_t size;                    // length of list

    // loop over map
    using KeyValue = std::pair<StringRef, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues mapValues;            // values to iterate over
    MapValues::iterator mapIt;      // iterator over values
  };

  SmallVector<LoopLevel, 0> m_loopStack;
  const json* m_data;

  SmallVector<const json*, 4> m_tmpArgs;
  json m_tmpVal;
};

}  // namespace inja

#endif  // WPIUTIL_INJA_RENDERER_H_
