# Implicit False for Missing Variables Feature

## Overview

This feature adds the ability to treat undefined/missing variables as `false` in conditional expressions instead of throwing an error. This provides a more convenient syntax similar to languages like JavaScript or Python where undefined variables evaluate to falsy values.

## Motivation

Previously, to check if a nested property exists, you had to write:
```jinja
{% if existsIn(entity, "Person") %}
```

With this feature enabled, you can simply write:
```jinja
{% if entity.Person %}
```

If `entity.Person` is not defined, it will evaluate to `false` instead of throwing an error.

## Usage

### Enabling the Feature

This feature is **disabled by default** to maintain backward compatibility. To enable it:

```cpp
inja::Environment env;
env.set_implicit_false_for_missing_vars(true);
```

### Behavior

When enabled:
- **In conditionals (`{% if %}`)**: Missing variables evaluate to `false`
- **In expressions (`{{ }}`)**: Missing variables still throw an error
- **Existing variables**: Evaluated normally using inja's truthy rules:
  - `false`, `null`, `0`, empty arrays/objects/strings → false
  - Everything else → true

### Examples

```cpp
inja::Environment env;
env.set_implicit_false_for_missing_vars(true);

inja::json data;
data["entity"] = {{"name", "John"}};

// Missing variable evaluates to false
env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data);
// Output: "no"

// Existing variable with truthy value
data["entity"]["Person"] = true;
env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data);
// Output: "yes"

// Existing variable with falsy value
data["entity"]["Person"] = false;
env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data);
// Output: "no"

// Still throws in expression context
env.render("{{ entity.Person }}", data);
// Throws: variable 'entity.Person' not found
```

### Testing

A comprehensive test suite is provided in `test/test-implicit-false.cpp` with the following subcases:
- `default behavior - throws on missing variable`
- `enabled - missing variables evaluate to false`
- `enabled - existing variables still work correctly`
- `enabled - expressions still throw on missing variables`
- `enabled - works with logical operators`
- `enabled - works with else if`
- `enabled - works in loops`

The tests are integrated into the main test suite via `test/test.cpp`.

Run the tests using the standard inja test process (requires CMake):
```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build
```

Or compile and run manually:
```bash
g++ -std=c++17 -D__TEST_DIR__=$(pwd)/test -I include -I third_party/include test/test.cpp -o test-runner
./test-runner --test-case="implicit_false_for_missing_vars"
```

## Backward Compatibility

This feature is **100% backward compatible** because:
- The flag defaults to `false`, preserving existing behavior
- No existing code is affected unless explicitly enabled
- When disabled, all original error-checking behavior remains intact

## Future Enhancements

Potential improvements could include:
- Per-template configuration instead of global environment setting
- More granular control over which contexts allow missing variables
- Integration with the `default` filter for fallback values
