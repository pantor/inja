[<div align="center"><img width="500" src="https://raw.githubusercontent.com/pantor/inja/master/doc/logo.jpg"></div>](https://github.com/pantor/inja/releases)



[![Build Status](https://travis-ci.org/pantor/inja.svg?branch=master)](https://travis-ci.org/pantor/inja)
[![Build status](https://ci.appveyor.com/api/projects/status/qtgniyyg6fn8ich8?svg=true)](https://ci.appveyor.com/project/pantor/inja)
[![Coverage Status](https://img.shields.io/coveralls/pantor/inja.svg)](https://coveralls.io/r/pantor/inja)
[![Codacy Status](https://api.codacy.com/project/badge/Grade/aa2041f1e6e648ae83945d29cfa0da17)](https://www.codacy.com/app/pantor/inja?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pantor/inja&amp;utm_campaign=Badge_Grade)
[![Github Issues](https://img.shields.io/github/issues/pantor/inja.svg)](http://github.com/pantor/inja/issues)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/pantor/inja/master/LICENSE)


Inja is a template engine for modern C++, loosely inspired by [jinja](http://jinja.pocoo.org) for python. It has an easy and yet powerful template syntax with all variables, loops, conditions, includes, blocks, comments you need, nested and combined as you like. The rendering syntax is works like magic and uses the wonderful [json](https://github.com/nlohmann/json) library by nlohmann for data input. Most importantly, *inja* needs only two header files, which is (nearly) as trivial as integration in C++ can get. Of course, everything is tested on all relevant compilers. Have a look what it looks like:

```c++
json data;
data["name"] = "world";

inja::render("Hello {{ name }}!", data); // "Hello World!"
```


## Integration

Inja is a headers only library, which can be downloaded in the releases or directly from the `src/` folder. Inja uses json by nlohmann as its single dependency, so make sure that it is included before inja. json can be downloaded.

```c++
#include "json.hpp"
#include "inja.hpp"

// For convenience
using namespace inja;
using json = nlohmann::json;
```


## Tutorial

### Template Rendering
```c++
json data;
data["name"] = "world";

render("Hello {{ name }}!", data); // "Hello World!"

// For more advanced usage, an environment is recommended
Environment env = Environment();

// Render a string with json data
std::string result = env.render("Hello {{ name }}!", data);

// Or directly read a template file
result = env.render_template("./template.txt", data);

// And read a json file for data
result = env.render_template("./template.txt", "./data.json");

// Or write a rendered template file
env.write("./template.txt", "./result.txt")
env.write("./template.txt", "./data.json", "./result.txt")
```

The environment class can be configured.
```c++
// With default settings
Environment env_default = Environment();

// With global path to template files
Environment env = Environment("../path/templates/");

// With global path where to save rendered files
Environment env = Environment("../path/templates/", "../path/results/");

// With other opening and closing strings (here the defaults)
env.setVariables("{{", "}}"); // Variables
env.setComments("{#", "#}"); // Comments
env.setStatements("{%", "%}"); // Statements for many things, see below
env.setLineStatements("##"); // Line statement (just an opener)
```

### Variables

Variables can be rendered within the `{{ ... }}` expressions.
```c++
json data;
data["neighbour"] = "Peter";
data["guests"] = {"Jeff", "Patrick", "Tom"};
data["time"]["start"] = 16;
data["time"]["end"] = 22;

// Indexing in array
render("{{ guests/1 }}", data); // "Patrick"

// Objects
render("{{ time/start }} to {{ time/end }}pm"); // "16 to 22pm"
```
In general, the variables can be fetched using the [JSON Pointer](https://tools.ietf.org/html/rfc6901) syntax. For convenience, the leading `/` can be ommited. If no variable is found, valid JSON is printed directly, otherwise an error is thrown.


### Statements

Statements can be written with the `{% ... %}` syntax. The most important statements are loops, conditions and file includes. All statements can be nested.

#### Loops

```c++
// Combining loops and line statements
render(R"(Guest List:
## for guest in guests
	{{ index1 }}: {{ guest }}
## endfor )", data)

/* Guest List:
	1: Jeff
	2: Pierre
	3: Tom */
```
In a loop, the special variables `number index`, `number index1`, `bool is_first` and `bool is_last` are available.

#### Conditions

Conditions support if, else if and else statements. Following conditions for example:
```c++
// Standard comparisons with variable
render("{% if time/hour >= 18 %}…{% endif %}", data); // True

// Variable in list
render("{% if neighbour in guests %}…{% endif %}", data); // True

// Logical operations
render("{% if guest_count < 5 and all_tired %}…{% endif %}", data); // True

// Negations
render("{% if not guest_count %}…{% endif %}", data); // True
```

#### Includes

Include other files, relative from the current file location.
```
{% include "footer.html" %}
```

### Functions

A few functions are implemented within the inja template syntax. They can be called with
```c++
// upper(<string>)
render("Hello {{ upper(neighbour) }}!", data); // "Hello PETER!"
render("Hello {{ lower(neighbour) }}!", data); // "Hello peter!"

// Range function, useful for loops
render("{% for i in range(4) %}{{ index1 }}{% endfor %}", data); // "1234"

// Length function (but please don't combine with range, use list directly...)
render("I count {{ length(guests) }} guests.", data); // "I count 3 guests."
```

### Comments

Comments can be written with the `{# ... #}` syntax.
```c++
render("Hello{# Todo #}!", data); // "Hello!"
```



## Supported compilers

Currently, the following compilers are tested:

- GCC 4.9 - 7.1 (and possibly later)
- Clang 3.6 - 3.7 (and possibly later)
- Microsoft Visual C++ 2015 / Build Tools 14.0.25123.0 (and possibly later)
- Microsoft Visual C++ 2017 / Build Tools 15.1.548.43366 (and possibly later)



## License

The class is licensed under the [MIT License](https://raw.githubusercontent.com/pantor/inja/master/LICENSE).
