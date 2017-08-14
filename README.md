# Inja

[![Build Status](https://travis-ci.org/pantor/inja.svg?branch=master)](https://travis-ci.org/pantor/inja)
[![Coverage Status](https://img.shields.io/coveralls/pantor/inja.svg)](https://coveralls.io/r/pantor/inja)
[![Codacy Status](https://api.codacy.com/project/badge/Grade/aa2041f1e6e648ae83945d29cfa0da17)](https://www.codacy.com/app/pantor/inja?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=pantor/inja&amp;utm_campaign=Badge_Grade)
[![Github Issues](https://img.shields.io/github/issues/pantor/inja.svg)](http://github.com/pantor/inja/issues)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/pantor/inja/master/LICENSE)


## Design

```c++
json data;
data["name"] = "world";

inja::render("Hello {{ name }}!", data); // "Hello World!"
```

## Integration

Inja is headers only. Just one dependency: json by nlohmann. 

```c++
#include "json.hpp"
#include "inja.hpp"
```
    

## Tutorial


### Template Rendering
```c++
json data;
data["name"] = "world";

inja::render("Hello {{ name }}!", data); // "Hello World!"

// For more advanced usage, an environment is recommended
inja::Environment env = inja::Environment();

// Render a string with json data
std::string result = env.render("Hello {{ name }}!", data);

// Or directly read a template file
std::string result_template = env.render_temlate("template.txt", data);

// And read a json file for data
std::string result_template_2 = env.render_temlate_with_json_file("template.txt", "data.json");
```

The environment class can be configured.
```c++
// With default settings
inja::Environment env_default = inja::Environment();

// With global path to template files
inja::Environment env_default = inja::Environment("../path/templates/"); 
```

### Variables

Variables can be rendered with the `{{ ... }}` syntax.


### Statements

Statements can be written with the `(% ... %)` syntax. The most important statements are loops, conditions and file includes.All statements can be nested.

#### Loops

<table>
	<tbody>
		<tr>
      		<th>Template</th>
      		<th>Json</th>
      		<th>Result</th>
    	</tr>
		<tr>
			<td>
<pre lang="txt">
Guests:
(% for guest in guests %){{ index1 }}: {{ guest }}
(% endfor %)</pre>
			</td>
			<td>
<pre lang="json">
{
  "guests":  [
    "Jeff",
    "Pierre",
    "Tom"
  ]
}</pre>
			</td>
			<td>
<pre lang="txt">
Guests:
1. Jeff
2. Pierre
3. Tom
</pre>
			</td>
		</tr>
	</tbody>
</table>

In the loop, some special variables are available:
- int index
- bool is_first
- bool is_last

#### Conditions

#### Includes

Include other files like `(% include "footer.html" %)`. Relative from file.

### Comments

Comments can be rendered with the `{# ... #}` syntax.

```c++
inja::render("Hello{# Todo #}!", data); // "Hello!"
```

## Supported compilers

Currently, the following compilers are tested:

- GCC 4.9 - 7.1 (and possibly later)
- Clang 3.6 - 3.7 (and possibly later)


## License

The class is licensed under the [MIT License](https://raw.githubusercontent.com/pantor/inja/master/LICENSE).



