# Inja

A Template Engine for Modern C++

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/aa2041f1e6e648ae83945d29cfa0da17)](https://www.codacy.com/app/pantor/inja?utm_source=github.com&utm_medium=referral&utm_content=pantor/inja&utm_campaign=badger)
[![Build Status](https://travis-ci.org/pantor/inja.svg?branch=master)](https://travis-ci.org/pantor/inja)
[![Coverage Status](https://img.shields.io/coveralls/pantor/inja.svg)](https://coveralls.io/r/pantor/inja)
[![Github Issues](https://img.shields.io/github/issues/pantor/inja.svg)](http://github.com/pantor/inja/issues)
[![GitHub License](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/pantor/inja/master/LICENSE)

```c++
json data = {{"name", "world"}};
string result = inja::render("Hello {{ world }}!", data);
// "Hello World!"
```

## Integration

Inja is headers only. Just one dependency: json by nlohmann. 

```c++
#include "json.hpp"
#include "inja.hpp"
```
    

## Examples


## Supported compilers

Currently, the following compilers are tested:

- GCC 4.9 - 7.1 (and possibly later)
- Clang 3.6 - 3.7 (and possibly later)

## License

The class is licensed under the [MIT License](https://raw.githubusercontent.com/pantor/inja/master/LICENSE).