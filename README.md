# libArgs

[![Travis (.org)](https://img.shields.io/travis/mbits-libs/libargs)](https://travis-ci.org/mbits-libs/libargs) [![Coveralls github](https://img.shields.io/coveralls/github/mbits-libs/libargs)](https://coveralls.io/github/mbits-libs/libargs?branch=master)

Small library for program argument parser, inspired by Python's `argparse`, depending only on the standard library, with C++17 as minimum requirement.

It automaticaly builds and prints out help info for program arguments, can react to missing or unrecognized arguments and has support for arguments with and without values.

The argument values can be stored in:
- `std::string`s,
- things, which can be constructed from strings (excluding `std::string_view`) &mdash; for example `std::filesystem::path`,
- all things `int` (uses `std::is_integral`, but excludes `bool`), as well as
- `std::optional`, `std::vector` and `std::unordered_set` of the things on this list.

## Building

```sh
mkdir build && cd build
cmake .. -G Ninja -DLIBARGS_TESTING=OFF -DLIBARGS_INSTALL=OFF
ninja
```

## Testing

```sh
mkdir build && cd build
cmake .. -G Ninja -DLIBARGS_TESTING=ON
ninja && ninja test
```

## Installing

```sh
mkdir build && cd build
cmake .. -G Ninja -DLIBARGS_TESTING=OFF -DLIBARGS_INSTALL=ON
ninja && sudo ninja install
```

## Example

To mimic Python example:

```cxx
#include <args/parser.hpp>
#include <numeric>
#include <vector>

unsigned accumulate(std::vector<unsigned> const& integers, auto pred) {
    return std::accumulate(integers.begin(), integers.end(), 0u, pred);
}

int main(int argc, char* argv[]) {
    std::vector<unsigned> integers{};
    bool is_sum{false};

    args::null_translator tr{};
    args::parser parser{"Process some integers.",
        args::from_main(argc, argv), &tr};
    parser.set<std::true_type>(is_sum, "sum")
        .opt()
        .help("sum the integers (default: find the max)");
    parser.arg(integers)
        .meta("N")
        .help("an integer for the accumulator");

    parser.parse();

    auto const result =
        is_sum ? accumulate(integers, [](auto a, auto b) { return a + b; })
               : accumulate(integers,
                            [](auto a, auto b) { return std::max(a, b); });
    printf("%u\n", result);
}
```
Assuming the code above is compiled to `prog`, it can be used to provide help information:

```
$ ./prog -h
usage: prog [-h] [--sum] N [N ...]

Process some integers.

positional arguments:
 N          an integer for the accumulator

optional arguments:
 -h, --help show this help message and exit
 --sum      sum the integers (default: find the max)
```

When run with arguments, it prints either the sum, or max, or complains about the command line:

```
$ ./prog 1 2 3 4
4
```
```
$ ./prog 1 2 3 4 --sum
10
```
```
$ ./prog a b c
usage: prog [-h] [--sum] N [N ...]
prog: error: argument N: expected a number
```
```
$ ./prog --sum
usage: prog [-h] [--sum] N [N ...]
prog: error: argument N is required
```

# Classes

## args::arglist

Extremely simple class, which would be `std::span<char const*>`, if this was targeting C++20 alone.

## args::args_view

A structure holding program name and argument list.

### args::from_main

```cxx
args_view from_main(int, char**);
args_view from_main(arglist const&);
```

Free function constructing `args_view` objects. Using the first element, locates the filename of the program and (on Windows) removes extension. Shifts left remaining arguments. Uses program filename and shifted arguments to create the `args_view`.

## args::base_translator, args::null_translator

Translator is a class with an `operator()`, which takes a message id and produces appropriate translation.

### null_translator::operator()

```cxx
std::string operator()(lng id,
                       std::string_view arg1 = {},
                       std::string_view arg2 = {}) const override;
```

Produces built in translations for given string ids:

|lng|result|
|---|------|
|`usage`|`"usage: "`|
|`def_meta`|`"ARG"`|
|`positionals`|`"positional arguments"`|
|`optionals`|`"optional arguments"`|
|`help_description`|`"show this help message and exit"`|
|`unrecognized`|`"unrecognized argument: $arg1"`|
|`needs_param`|`"argument $arg1: expected one argument"`|
|`needs_number`|`"argument $arg1: expected a number"`|
|`needed_number_exceeded`|`"argument $arg1: number outside of expected bounds"`|
|`required`|`"argument $arg1 is required"`|
|`error_msg`|`"$arg1: error: $arg2"`|

## args::actions::builder

Returned from `parser::set<Value>`, `parser::add` and `parser::custom`, allows to tweak the argument in addition to what was provided in said `parser` methods.

```cxx
unsigned verbosity{3};
parser.custom([&]{ ++verbosity; }, "v")
      .opt()
      .multi()
      .help("increases debug log level");
```

### builder::meta

```cxx
builder& builder::meta(std::string_view name);
```

Provides a name for the value in arguments with value for both `parser::help` and `parser::short_help`. Defaults to value translated from `lng::def_meta`.

### builder::help

```cxx
builder& builder::help(std::string_view dscr);
```

Provides help message for the `parser::help` output.

### builder::multi

```cxx
builder& builder::multi(bool value = true);
```

Provides the cardinality hint for `parser::short_help`; otherwise not used. Defaults to `true` for `std::vector` and `std::unordered_set` and to `false` otherwise.

### builder::req, builder::opt

```cxx
builder& builder::req(bool value = true);
builder& builder::opt(bool value = true); // same as "req(!value)"
```

Provide the cardinality hint for `parser::short_help` and aid in `parser::parse()` aftermath. The underlying attribute, "required", defaults to `false` for `std::optional` and to `true` otherwise.

## args::parser

Main class of the library.

### parser::parser

```cxx
parser(std::string description, args_view const& args, base_translator const* tr);
parser(std::string description, std::string_view progname, arglist const& args, base_translator const* tr);
parser(std::string description, arglist const& args, base_translator const* tr);
```

Constructs a parser with given description, arguments and translator. Second version packs `progname` and `args` into `args_view` and calls first version. Third version uses `from_main` helper and calls first version.

### parser::set\<Value\>

```cxx
template <typename Value, typename Storage, typename... Names>
actions::builder set(Storage& dst, Names&&... names);
```

Creates an argument, which does not need a value and assumes `Value` is `std::integral_constant`-like class by setting the `dst` with `Value::value`, when triggered. Useful with boolean flags. In such scenario, the variable behind would be set to one value and the opposite type would be selected for the `set`:

```cxx
bool foo{true};
bool bar{false};

parser.set<std::false_type>(foo, "no-foo");
parser.set<std::true_type>(bar, "bar");
```

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

### parser::arg

```cxx
template <typename Storage, typename... Names>
actions::builder arg(Storage& dst, Names&&... names);
```

Creates an argument with a value. If the `Storage` type is `std::optional`, the argument will not be required. If the `Storage` type is either `std::vector` or `std::unordered_set`, the created argument will add new items to the container.

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

### parser::custom

```cxx
template <typename Callable, typename... Names>
actions::builder custom(Callable cb, Names&&... names);
```

Creates an argument with custom callback. If the callback does not take any arguments, or takes a single `args::parser&` argument, then this is an argument without a value. If the callback takes either string type, or `args::parser&` followed by a string type, then this is an argument with a value. String type is one of `std::string`, `std::string_view`, `std::string const&` or `std::string_view const&`.

`Names`, if given, will be used for argument names. One-letter names will create single-dash arguments, longer names will create double-dash arguments. Empty `names` list will result in positional argument.

```cxx
parser.custom([]() { ... }, "foo", "f"); // used as: "--foo" or "-f"
parser.custom([](std::string_view arg) { ... }, "Z"); // used as: "-Z qux"
parser.custom([](args::parser& p) { ... }, "A"); // used as: "-A"
parser.custom([](args::parser& p, std::string_view arg) { ... }, "bar"); // used as: "--bar baz"
```

### parser::parse

```cxx
arglist parse(unknown_action on_unknown = exclusive_parser,
              std::optional<size_t> maybe_width = {});
```

Parses the arguments. The `on_unknown` instructs the parser, how to react on an argument without matching action. The default `parser::exclusive_parser` will print an error and terminate program; `parser::allow_subcommands` will stop processing and will return remaining arguments.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::provide_help

```cxx
void provide_help(bool value = true);
bool provide_help() const noexcept;
```

Turns off/on the built in support for `"-h"` and `"--help"` arguments.

### parser::short_help

```cxx
void short_help(FILE* out = stdout,
                bool for_error = false,
                std::optional<size_t> maybe_width = {}) const;
```

Prints "usage" line, with program name and possible arguments and their optionality/cardinality.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::help

```cxx
[[noreturn]] void help(std::optional<size_t> maybe_width = {}) const;
```

Prints `short_help()` and follows with description and arguments, finally calls `std::exit(0)`.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.

### parser::error

```cxx
[[noreturn]] void error(std::string const& msg,
                        std::optional<size_t> maybe_width = {}) const;
```

Prints `short_help()` and follows with error message, finally calls `std::exit` with non-zero value.

The `maybe_width` is a hint for terminal width. If missing, will try to get the current width of a terminal. If the program is not attached to terminal, will print the messages without any formatting.
