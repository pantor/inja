[<div align="center"><img width="500" src="https://raw.githubusercontent.com/pantor/inja/master/doc/logo.jpg"></div>](https://github.com/pantor/inja/releases)


[![Build Status](https://travis-ci.org/pantor/inja.svg?branch=master)](https://travis-ci.org/pantor/inja)
[![Build status](https://ci.appveyor.com/api/projects/status/qtgniyyg6fn8ich8/branch/master?svg=true)](https://ci.appveyor.com/project/pantor/inja)
[![Coverage Status](https://img.shields.io/coveralls/pantor/inja.svg)](https://coveralls.io/r/pantor/inja)
[![Codacy Status](https://api.codacy.com/project/badge/Grade/aa2041f1e6e648ae83945d29cfa0da17)](https://www.codacy.com/app/pantor/inja?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pantor/inja&amp;utm_campaign=Badge_Grade)
[![Github Releases](https://img.shields.io/github/release/pantor/inja.svg)](https://github.com/pantor/inja/releases)
[![Github Issues](https://img.shields.io/github/issues/pantor/inja.svg)](http://github.com/pantor/inja/issues)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/pantor/inja/master/LICENSE)


Inja is a template engine for modern C++, loosely inspired by [jinja](http://jinja.pocoo.org) for python. It has an easy and yet powerful template syntax with all variables, loops, conditions, includes, callbacks, comments you need, nested and combined as you like. Inja uses the wonderful [json](https://github.com/nlohmann/json) library by nlohmann for data input and handling. Most importantly, *inja* needs only two header files, which is (nearly) as trivial as integration in C++ can get. Of course, everything is tested on all relevant compilers. Here is what it looks like:

```c++
json data;
data["name"] = "world";

inja::render("Hello {{ name }}!", data); // Returns "Hello world!"
```

## Integration

Inja is a headers only library, which can be downloaded from the [releases](https://github.com/pantor/inja/releases) or directly from the `include/` or `single_include/` folder. Inja uses `nlohmann/json.hpp` as its single dependency, so make sure it can be included from `inja.hpp`. json can be downloaded [here](https://github.com/nlohmann/json/releases). Then integration is as easy as:

```c++
#include <inja.hpp>

// Just for convenience
using namespace inja;
using json = nlohmann::json;
```

If you are using the [Meson Build System](http://mesonbuild.com), then you can wrap this repository as a subproject.

If you are using [Conan](https://conan.io) to manage your dependencies, have a look at [this repository](https://github.com/DEGoodmanWilson/conan-inja). Please file issues [here](https://github.com/DEGoodmanWilson/conan-inja/issues) if you experience problems with the packages.

You can also integrate inja in your project using [Hunter](https://github.com/ruslo/hunter), a package manager for C++.

If you are using [vcpkg](https://github.com/Microsoft/vcpkg) on your project for external dependencies, then you can use the [inja package](https://github.com/Microsoft/vcpkg/tree/master/ports/inja). Please see the vcpkg project for any issues regarding the packaging.

If you are using [cget](https://cget.readthedocs.io/en/latest/), you can install the latest development version with `cget install pantor/inja`. A specific version can be installed with `cget install pantor/inja@v2.1.0`.


## Tutorial

This tutorial will give you an idea how to use inja. It will explain the most important concepts and give practical advices using examples and executable code. Beside this tutorial, you may check out the [documentation](https://pantor.github.io/inja).

### Template Rendering

The basic template rendering takes a template as a `std::string` and a `json` object for all data. It returns the rendered template as an `std::string`.

```c++
json data;
data["name"] = "world";

render("Hello {{ name }}!", data); // Returns std::string "Hello world!"
render_to(std::cout, "Hello {{ name }}!", data); // Prints "Hello world!"
```

For more advanced usage, an environment is recommended.
```c++
Environment env;

// Render a string with json data
std::string result = env.render("Hello {{ name }}!", data); // "Hello world!"

// Or directly read a template file
Template temp = env.parse_template("./template.txt");
std::string result = env.render(temp, data); // "Hello world!"

data["name"] = "Inja";
std::string result = env.render(temp, data); // "Hello Inja!"

// Or read a json file for data directly from the environment
result = env.render_file("./template.txt", "./data.json");

// Or write a rendered template file
env.write(temp, data, "./result.txt");
env.write_with_json_file("./template.txt", "./data.json", "./result.txt");
```

The environment class can be configured to your needs.
```c++
// With default settings
Environment env_default;

// With global path to template files and where files will be saved
Environment env_1 {"../path/templates/"};

// With separate input and output path
Environment env_2 {"../path/templates/", "../path/results/"};

// Choose between dot notation (like Jinja2) and JSON pointer to access elements
env.set_element_notation(ElementNotation::Dot); // (default) e.g. time.start
env.set_element_notation(ElementNotation::Pointer); // e.g. time/start

// With other opening and closing strings (here the defaults)
env.set_expression("{{", "}}"); // Expressions
env.set_comment("{#", "#}"); // Comments
env.set_statement("{%", "%}"); // Statements {% %} for many things, see below
env.set_line_statement("##"); // Line statements ## (just an opener)
```

### Variables

Variables are rendered within the `{{ ... }}` expressions.
```c++
json data;
data["neighbour"] = "Peter";
data["guests"] = {"Jeff", "Tom", "Patrick"};
data["time"]["start"] = 16;
data["time"]["end"] = 22;

// Indexing in array
render("{{ guests.1 }}", data); // "Tom"

// Objects
render("{{ time.start }} to {{ time.end }}pm", data); // "16 to 22pm"
```
In general, the variables can be fetched using the [JSON Pointer](https://tools.ietf.org/html/rfc6901) syntax. For convenience, the leading `/` can be ommited. If no variable is found, valid JSON is printed directly, otherwise an error is thrown.


### Statements

Statements can be written either with the `{% ... %}` syntax or the `##` syntax for entire lines. The most important statements are loops, conditions and file includes. All statements can be nested.

#### Loops

```c++
// Combining loops and line statements
render(R"(Guest List:
## for guest in guests
	{{ loop.index1 }}: {{ guest }}
## endfor )", data)

/* Guest List:
	1: Jeff
	2: Tom
	3: Patrick */
```
In a loop, the special variables `loop/index (number)`, `loop/index1 (number)`, `loop/is_first (boolean)` and `loop/is_last (boolean)` are defined. In nested loops, the parent loop variables are available e.g. via `loop/parent/index`. You can also iterate over objects like `{% for key, value in time %}`.

#### Conditions

Conditions support the typical if, else if and else statements. Following conditions are for example possible:
```c++
// Standard comparisons with variable
render("{% if time.hour >= 18 %}…{% endif %}", data); // True

// Variable in list
render("{% if neighbour in guests %}…{% endif %}", data); // True

// Logical operations
render("{% if guest_count < 5 and all_tired %}…{% endif %}", data); // True

// Negations
render("{% if not guest_count %}…{% endif %}", data); // True
```

#### Includes

You can either include other template files or already parsed templates.
```
// Other template files are included relative from the current file location
render({% include "footer.html" %}, data);

// To include in-memory templates, add them to the environment first
env.include_template("footer", temp);
render({% include "footer" %}, data);
```

### Functions

A few functions are implemented within the inja template syntax. They can be called with
```c++
// Upper and lower function, for string cases
render("Hello {{ upper(neighbour) }}!", data); // "Hello PETER!"
render("Hello {{ lower(neighbour) }}!", data); // "Hello peter!"

// Range function, useful for loops
render("{% for i in range(4) %}{{ loop.index1 }}{% endfor %}", data); // "1234"

// Length function (please don't combine with range, use list directly...)
render("I count {{ length(guests) }} guests.", data); // "I count 3 guests."

// Get first and last element in a list
render("{{ first(guests) }} was first.", data); // "Jeff was first."
render("{{ last(guests) }} was last.", data); // "Patir was last."

// Sort a list
render("{{ sort([3,2,1]) }}", data); // "[1,2,3]"
render("{{ sort(guests) }}", data); // "[\"Jeff\", \"Patrick\", \"Tom\"]"

// Round numbers to a given precision
render("{{ round(3.1415, 0) }}", data); // 3
render("{{ round(3.1415, 3) }}", data); // 3.142

// Check if a value is odd, even or divisible by a number
render("{{ odd(42) }}", data); // false
render("{{ even(42) }}", data); // true
render("{{ divisibleBy(42, 7) }}", data); // true

// Maximum and minimum values from a list
render("{{ max([1, 2, 3]) }}", data); // 3
render("{{ min([-2.4, -1.2, 4.5]) }}", data); // -2.4

// Convert strings to numbers
render("{{ int(\"2\") == 2 }}", data); // true
render("{{ float(\"1.8\") > 2 }}", data); // false

// Set default values if variables are not defined
render("Hello {{ default(neighbour, \"my friend\") }}!", data); // "Hello Peter!"
render("Hello {{ default(colleague, \"my friend\") }}!", data); // "Hello my friend!"

// Check if a key exists in an object
render("{{ exists(\"guests\") }}", data); // "true"
render("{{ exists(\"city\") }}", data); // "false"
render("{{ existsIn(time, \"start\") }}", data); // "true"
render("{{ existsIn(time, neighbour) }}", data); // "false"

// Check if a key is a specific type
render("{{ isString(neighbour) }}", data); // "true"
render("{{ isArray(guests) }}", data); // "true"
// Implemented type checks: isArray, isBoolean, isFloat, isInteger, isNumber, isObject, isString,
```

### Callbacks

You can create your own and more complex functions with callbacks.
```c++
Environment env;

/*
 * Callbacks are defined by its:
 * - name
 * - number of arguments
 * - callback function. Implemented with std::function, you can for example use lambdas.
 */
env.add_callback("double", 1, [](Arguments& args) {
	int number = args.at(0)->get<int>(); // Adapt the index and type of the argument
	return 2 * number;
});

// You can then use a callback like a regular function
env.render("{{ double(16) }}", data); // "32"

// A callback without argument can be used like a dynamic variable:
std::string greet = "Hello";
env.add_callback("double-greetings", 0, [greet](Arguments args) {
	return greet + " " + greet + "!";
});
env.render("{{ double-greetings }}", data); // "Hello Hello!"
```

### Comments

Comments can be written with the `{# ... #}` syntax.
```c++
render("Hello{# Todo #}!", data); // "Hello!"
```



## Supported compilers

Inja uses `string_view` from C++17, but includes the [polyfill](https://github.com/martinmoene/string-view-lite) from martinmoene. This way, the minimum version is C++11. Currently, the following compilers are tested:

- GCC 5.0 - 8.0 (and possibly later)
- Clang 5.0 - 6.0 (and possibly later)
- Microsoft Visual C++ 2015 - 2017 (and possibly later)
