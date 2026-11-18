/*
  ___        _          Version 3.5.0
 |_ _|_ __  (_) __ _    https://github.com/pantor/inja
  | || '_ \ | |/ _` |   Licensed under the MIT License <http://opensource.org/licenses/MIT>.
  | || | | || | (_| |
 |___|_| |_|/ |\__,_|   Copyright (c) 2018-2025 Lars Berscheid
          |__/
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

module;

#include "inja/inja.hpp"

export module inja;

export namespace inja {
    using inja::Arguments;
    using inja::BlockNode;
    using inja::BlockStatementNode;
    using inja::CallbackFunction;
    using inja::DataError;
    using inja::DataNode;
    using inja::Environment;
    using inja::ExpressionListNode;
    using inja::ExpressionNode;
    using inja::ExtendsStatementNode;
    using inja::FileError;
    using inja::ForArrayStatementNode;
    using inja::ForObjectStatementNode;
    using inja::ForStatementNode;
    using inja::FunctionNode;
    using inja::FunctionStorage;
    using inja::IfStatementNode;
    using inja::IncludeStatementNode;
    using inja::InjaError;
    using inja::Lexer;
    using inja::LexerConfig;
    using inja::LiteralNode;
    using inja::NodeVisitor;
    using inja::Parser;
    using inja::ParserConfig;
    using inja::ParserError;
    using inja::RenderConfig;
    using inja::RenderError;
    using inja::Renderer;
    using inja::SetStatementNode;
    using inja::SourceLocation;
    using inja::StatementNode;
    using inja::StatisticsVisitor;
    using inja::Template;
    using inja::TemplateStorage;
    using inja::TextNode;
    using inja::Token;
    using inja::VoidCallbackFunction;

    using inja::get_source_location;
    using inja::htmlescape;
    using inja::json;
    using inja::render;
    using inja::render_to;
    using inja::replace_substring;

    namespace string_view {
        using inja::string_view::slice;
        using inja::string_view::split;
        using inja::string_view::starts_with;
    }
}
