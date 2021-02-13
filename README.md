- [Use](#use)
- [Moving types in and out of a typelist](#moving-types-in-and-out-of-a-typelist)
- [Modifying a typelist](#modifying-a-typelist)
  - [any_of, all_of, none_of](#any_of-all_of-none_of)
  - [find_if](#find_if)
  - [filter](#filter)
  - [contains](#contains)
  - [sort](#sort)
  - [transform, transform_with, transform_v](#transform-transform_with-transform_v)
  - [set](#set)
  - [at](#at)
  - [slice](#slice)
  - [size](#size)
- [Example - Generate a variant, with no duplicate types, that holds any value from a tuple](#example---generate-a-variant-with-no-duplicate-types-that-holds-any-value-from-a-tuple)
- [API Samples](#api-samples)

# cpp-typelist  <!-- omit in toc -->

```c++
#include <tuple>
#include <string>
#include <utility>
#include <variant>

#include "dhagedorn/types/typelist.hh"

using namespace dhagedorn::types;

using record = std::tuple<std::string, int, std::string, int>;
using record_field = typelist<>
    ::from<record>
    ::set
    ::push_front<std::monostate>
    ::as<std::variant>;
    // std::variant<std::monostate, std::string, int>
```

A functional-style typelist for C++20.

Requires a C++20 compiler.  GCC 10.1.0 and newer are known to work

## Use

Create a typelist with

```c++
using tl = typelist<TYPE_1, TYPE_2, ...>;
```

## Moving types in and out of a typelist

Create a typelist by extracting the types from another type with `from`:
```c++
using number = std::variant<int, float, double>;
using number_types = typelist<>::from<number>;
// typelist<int, float, double>
```

Convert a typelist to a usable type with `as`:
```c++
using number_types = typelist<>::from<number>;
using number = typelist<number_types>::as<std::variant>;
// std::variant<int, float, double>
```

## Modifying a typelist

Supported operations are:
```
any_of
all_of
none_of
find_if
filter
contains
sort
transform
transform_with
transform_v
set
push_back
push_front
at
slice
size
trait_adapter
```

I've tried to follow names from the STL and the excellent [range-v3](https://github.com/ericniebler/range-v3).

Unless otherwise stated, all algorithms accept a predicate of type `iter_predicate`:
```c++
[]<typename T>(){ return /** true/false based on T */; }`
```

An overloaded form is also available:
```c++
[]<typename T, std::size_t /*or auto*/ I>(){ ...; };
```

std type_traits can also be converted to predicates:

```c++
using only_ints = typelist<int, float, short>
    ::filter<trait_adapter<std::is_integral>>;
    // typelist<int, short>
```

### any_of, all_of, none_of

Equivalents of the [std library's](https://en.cppreference.com/w/cpp/algorithm/all_any_none_of)

```c++
constexpr auto is_integral = typelist<int, float, char*>
    ::any_of<[]<typename T>(){ return std::is_integral_v<T>; }>;
    // true
```

### find_if

Equivalent to std library's [find_if](https://en.cppreference.com/w/cpp/algorithm/find)

```c++
using is_floating_point = typelist<int, float, char*>
    ::find_if<[]<typename T>(){ return std::is_floating_point<T>; }>;
    // float
```

### filter

Filter out unwanted types:

```c++
using only_numeric = typelist<int, float, char*>
    ::filter<[]<typename T>(){ return std::is_numeric_v<T>; }>;
    // typelist<int>
```

An overloaded predicate can also be used:
```c++
[]<typename T, std::size_t I, is_typelist CURRENT_LIST>[]( ...; );
```

`CURRENT_LIST` is the current value of the filtered output list.<br>
This can be used to implement more complex filters.<br>
See `set` for an example.

### contains

True if the list contains the provied type:

```c++
constexpr auto has_int = typelist<int, float, char*>
    ::contains<int>;
    // true
```

### sort

Sorts the typelist using a predicate implementing [strict weak ordering](https://en.cppreference.com/w/cpp/concepts/strict_weak_order):

```c++
[]<typename A, typename B>(){ return sizeof(A) < sizeof(B); };
```

`sort<>` is equivalent to calling:
```c++
using sorted = typelist<int, float, char*>::sort<[]<typename A, typename B>(){ return sizeof(A) < sizeof(B); };
// typelist<float, int, char*>
// (assuming 32 bit float, 64 bit int and pointers)
```
### transform, transform_with, transform_v

Convert one list into another

`transform` expects a target templated type:

```c++
using pointy_types = typelist<int, float, char*>
    ::transform<std::shared_ptr>;
    // typelist<shared_ptr<int>, shared_ptr<float>, shared_ptr<char>>
```

`transform_with` uses the return type of its predicate as
an expression to generate the new type:

```c++
using unsafe_pointy_ints = typelist<int, float, char*>
    ::transform_with<typename T>() -> T* {}>;
    // typelist<int*, float*, char**>
```
`transform_v` converts the typelist into a std::array:

```c++
constexpr std::array sizes = typelist<int, float, char*>
    ::transform_v<[]<typename T>{ return sizeof(T); }>;
    // std::array{ 4, 4, 8 }
```

### set

Generates a set (no duplicate types):

```c++
using record = std::tuple<int, float, char*, char*, float, int>;
using any_tuple_value = typelist<>
    ::from<record>
    ::set
    ::push_back<std::monostate>
    ::as<std::variant>;
    // std::variant<std::monostate, int, float, char*>
```

### at

Returns the type at the specified index

```c++
using first_type = typelist<int, float, char*>::at<0>; // int
```

### slice

Returns a slice of the typelist

```c++
using subset = typelist<int, float, char*>
    ::slice<0,2>;
    // typelist<int, float>
```

### size

Returns the number of types in the typelist

```c++
constexpr static auto size  = typelist<int, float, char*>::size // 3
```

## Example - Generate a variant, with no duplicate types, that holds any value from a tuple
```c++
#include <tuple>
#include <utility>
#include <variant>

#include "typelist.hh"

using namespace dhagedorn::types;

using record = std::tuple<std::string, int, std::string, int>;
using record_field = typelist<>
    ::from<record>
    ::set
    ::push_front<std::monostate>
    ::as<std::variant>; // std::variant<std::monostate, std::string, int>

record_field tuple_runtime_get(std::size_t index, const std::tuple& rec) {
    record_field out;

    [&]<auto ...INDICES>(std::index_sequence<INDICES...>) {
        auto test_element = [&]<auto I>() {
            if (index == I) {
                out = std::get<I>(rec);
            }
        };

        ((test_element.template operator()<INDICES>()) , ...);
    }(std::make_index_sequence<std::tuple_size_v<record>>());

    return out;
}

record row{"a", 1, "b", 2};
std::string value = std::get<std::string>(tuple_runtime_get(0, row));
```

## API Samples

see [sample.cc](sample/sample.cc)

```c++
   using list = types::typelist<double, float, int, char, int, char, float, double>;
    constexpr auto size = list::size;
    constexpr auto has_double = list::any_of<[]<typename T>(){ return std::is_same_v<T,double>; }>;
    constexpr auto is_mathy = list::any_of<[]<typename T>(){ return std::is_integral_v<T> || std::is_floating_point_v<T>; }>;
    constexpr auto is_not_stringy = list::none_of<[]<typename T>(){ return std::is_same_v<T, std::string>; }>;
    constexpr auto has_int = list::contains<int>;
    constexpr auto has_int2 = list::any_of<types::trait_predicate<std::is_integral>>;
    using with_string = list::push_back<std::string>;
    using with_void = list::push_front<void>;
    using set = list::set;
    using no_floats = list::filter<[]<typename T>(){ return !std::is_floating_point_v<T>; }>;
    using no_floats2 = list::filter<types::trait_predicate<std::is_integral>>;
    using odds = list::filter<[]<typename T, auto I, types::is_typelist list>(){ return I%2 == 1; }>;
    using sliced = list::slice<0,3>;
    using first_integral = list::find_if<[]<typename T>(){ return std::is_integral_v<T>; }>;
    using first_type = list::at<0>;
    constexpr auto sizes = list::transform_v<[]<typename T>(){ return sizeof(T); }>;
    constexpr auto indices = list::transform_v<[]<typename T, auto I>(){ return I; }>;
    using pointy = list::transform_with<[]<typename T>() -> T* { return nullptr; }>;
    using safe_pointy = list::transform<std::shared_ptr>;
    using sorted = list::sort<>;
    using sorted_backwards = list::sort<[]<typename A, typename B>(){ return sizeof(B) < sizeof(A); }>;
    using variant = list::as<std::variant>;
    using from_variant = list::from<variant>;
```

Evaluates to:

```c++
list:                dhagedorn::types::typelist<double, float, int, char, int, char, float, double>
size:                8
has_double:          true
is_mathy:            true
is_not_stringy:      true
has_int:             true
has_int2:            true
with_string:         dhagedorn::types::typelist<double, float, int, char, int, char, float, double, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >
with_void:           dhagedorn::types::typelist<void, double, float, int, char, int, char, float, double>
set:                 dhagedorn::types::typelist<double, float, int, char>
no_floats:           dhagedorn::types::typelist<int, char, int, char>
no_floats2:          dhagedorn::types::typelist<int, char, int, char>
odds:                dhagedorn::types::typelist<float, char, char, double>
sliced:              dhagedorn::types::typelist<double, float, int>
first_integral:      double
first_type:          double
sizes:               8, 4, 4, 1, 4, 1, 4, 8
indices:             0, 1, 2, 3, 4, 5, 6, 7
pointy:              dhagedorn::types::typelist<double*, float*, int*, char*, int*, char*, float*, double*>
safe_pointy:         dhagedorn::types::typelist<std::shared_ptr<double>, std::shared_ptr<float>, std::shared_ptr<int>, std::shared_ptr<char>, std::shared_ptr<int>, std::shared_ptr<char>, std::shared_ptr<float>, std::shared_ptr<double> >
sorted:              dhagedorn::types::typelist<char, char, float, int, int, float, double, double>
sorted_backwards:    dhagedorn::types::typelist<double, double, float, int, int, float, char, char>
variant:             std::variant<double, float, int, char, int, char, float, double>
from_variant:        dhagedorn::types::typelist<double, float, int, char, int, char, float, double>
```

