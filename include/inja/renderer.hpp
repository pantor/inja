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

#include <nlohmann/json.hpp>

#include <inja/internal.hpp>
#include <inja/template.hpp>


namespace inja {

std::string_view ConvertDotToJsonPointer(std::string_view dot, std::string& out);

class Renderer {
 public:
  Renderer(const TemplateStorage& includedTemplates, const FunctionStorage& callbacks): m_includedTemplates(includedTemplates), m_callbacks(callbacks) {
    m_stack.reserve(16);
    m_tmpArgs.reserve(4);
  }

  void render_to(std::stringstream& os, const Template& tmpl, const json& data);

 private:
  std::vector<const json*>& GetArgs(const Bytecode& bc) {
    m_tmpArgs.clear();

    bool hasImm = ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop);

    // get args from stack
    unsigned int popArgs = bc.args;
    if (hasImm) --popArgs;


    for (auto i = std::prev(m_stack.end(), popArgs); i != m_stack.end(); i++) {
      m_tmpArgs.push_back(&(*i));
    }

    // get immediate arg
    if (hasImm) {
      m_tmpArgs.push_back(GetImm(bc));
    }

    return m_tmpArgs;
  }

  void PopArgs(const Bytecode& bc) {
    unsigned int popArgs = bc.args;
    if ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop)
      --popArgs;
    for (unsigned int i = 0; i < popArgs; ++i) m_stack.pop_back();
  }

  const json* GetImm(const Bytecode& bc) {
    std::string ptrBuf;
    std::string_view ptr;
    switch (bc.flags & Bytecode::kFlagValueMask) {
      case Bytecode::kFlagValuePop:
        return nullptr;
      case Bytecode::kFlagValueImmediate:
        return &bc.value;
      case Bytecode::kFlagValueLookupDot:
        ptr = ConvertDotToJsonPointer(bc.str, ptrBuf);
        break;
      case Bytecode::kFlagValueLookupPointer:
        ptrBuf += '/';
        ptrBuf += bc.str;
        ptr = ptrBuf;
        break;
    }
    try {
      return &m_data->at(json::json_pointer(ptr.data()));
    } catch (std::exception&) {
      // try to evaluate as a no-argument callback
      if (auto cb = m_callbacks.FindCallback(bc.str, 0)) {
        std::vector<const json*> asdf{};
        m_tmpVal = cb(asdf, *m_data);
        return &m_tmpVal;
      }
      inja_throw("render_error", "variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  void UpdateLoopData()  {
    LoopLevel& level = m_loopStack.back();
    if (level.keyName.empty()) {
      level.data[level.valueName.data()] = *level.it;
      auto& loopData = level.data["loop"];
      loopData["index"] = level.index;
      loopData["index1"] = level.index + 1;
      loopData["is_first"] = (level.index == 0);
      loopData["is_last"] = (level.index == level.size - 1);
    } else {
      level.data[level.keyName.data()] = level.mapIt->first;
      level.data[level.valueName.data()] = *level.mapIt->second;
    }
  }

  const TemplateStorage& m_includedTemplates;
  const FunctionStorage& m_callbacks;

  std::vector<json> m_stack;

  struct LoopLevel {
    std::string_view keyName;       // variable name for keys
    std::string_view valueName;     // variable name for values
    json data;                      // data with loop info added

    json values;                    // values to iterate over

    // loop over list
    json::iterator it;              // iterator over values
    size_t index;                   // current list index
    size_t size;                    // length of list

    // loop over map
    using KeyValue = std::pair<std::string_view, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues mapValues;            // values to iterate over
    MapValues::iterator mapIt;      // iterator over values
  };

  std::vector<LoopLevel> m_loopStack;
  const json* m_data;

  std::vector<const json*> m_tmpArgs;
  json m_tmpVal;
};

}  // namespace inja

#endif  // WPIUTIL_INJA_RENDERER_H_
