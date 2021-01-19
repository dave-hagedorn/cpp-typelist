# cpp-typelist

A functional-style typelist for C++20.

Lambdas are used as predicates to algorithms (filter, transform, find_if, etc.)
that evaluate directly to new types, typelists, or constexpr values.

Rather than typing, say 

```c++
using only_ints = decltype(some_typelist<TYPES...>::filter([](auto& tag) { return std::is_integral_v<decltype(tag)>; }))
```

you can now type 

```c++
using only_ints = typelist<TYPES...>::filter<[]<typename T>() { return std::is_integral_v<T>; }>;
```

This allows chaining operations: 

```c++
using only_pointy_ints = typelist<TYPES...>
    ::filter<[]<typename T>() { return std::is_integral_v<T>; }>
    ::transform_with<[]<typename T>() -> T* {}>;
```

See [sample.cc](sample.cc).

#### Use

Create a typelist with `using tl = typelist<TYPE 1, TYPE 2, ...>;`.

#### Moving to and from a typelist

Create a typelist by extracting the types from another type using `from`: `using tl = typelist<>::from<variant<int, float, double>>;`.

Can convert a typelist to a usable type using `as`: `using numbers = typelist<int, float, double>::as<std::variant>;`

#### Algorithms

Supported algorithms are: `any_of, all_of, none_of, filter, sort, find_if, contains, and transform/transfor_with/transform_v`

I've tried to follow names from the STL and the excellent [range-v3](https://github.com/ericniebler/range-v3).

Most algorithms take an `iter_predicate` of form: `[]<typename T>(){ return /** some true/false based on T */; }`<br>
Ex: `using aligned_types = typelist<int, float, double>::filter<[]<typename T>(){ return sizeof(T) % 4 == 0; }`<br>
An overloaded form of `[]<typename T, std::size_t I>(){ ...; }` can also be used.

`filter` takes a `filter_predicate` of `[]<typename T>(){ ... }`.<br>
An overloaded form of `[]<typename T, std::size_t I, is_typelist CURRENT_LIST>{ ... }` is also allowed.<br>
`CURRENT_TYPELIST` is the current filtered list.<br>
See `set` for an example of this use

`transform` takes a target type, converting each type in the list into that type:<br> 
`using pointy_numbers = typelist<int, float, double>::transform<std::shared_ptr>`

`transform_with` takes a lambda and uses its output type as the target type.<br>
This allows building conversion expressions, such as:<br>
`using unsafe_pointye_ints = typelist<int, float, double>::transform_with<typename T>() -> T* {}> // typelist<int*, float*, double*>`

`transform_v` transforms the list into a value using an `iter_predicate`, emitting a std::array:<br>
`constexpr std::array sizes = typelist<int, float, double>::transform_v<[]<typename T>{ return sizeof(T); >`

`sort` takes a lambda comparing two types with strict weak ordering (A < B).<br>
`sort<>` is equivalent to calling:<br>
`using sorted = typelist<int, float, double>::sort<[]<typename A, typename B>(){ return sizeof(A) < sizeof(B); }`

#### Utilities

A typelist can be converted into a set using `typelist<>::set`.<br>
This is useful with `std::variant` for example, where duplicate types are supported by difficult to use.

Indexing into a typelist is done with `::at<I>`

Slice a list with `::slice<I1, I2>`

The size of the list is `::size`

Append or prepend types to a typelist using `::push_back<T>` and `::push_front<T>`

#### Ex - Generate a variant, with no duplicate types, that holds any value from a tuple
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

#### Complete API

```c++
using list = types::typelist<double, float, int, char, int, char, float, double>;
constexpr auto size = list::size;
constexpr auto has_double = list::any_of<[]<typename T>(){ return std::is_same_v<T,double>; }>;
constexpr auto is_mathy = list::any_of<[]<typename T>(){ return std::is_integral_v<T> || std::is_floating_point_v<T>; }>;
constexpr auto is_not_stringy = list::none_of<[]<typename T>(){ return std::is_same_v<T, std::string>; }>;
constexpr auto has_int = list::contains<int>;
using with_string = list::push_back<std::string>;
using with_void = list::push_front<void>;
using set = list::set;
using no_floats = list::filter<[]<typename T>(){ return !std::is_floating_point_v<T>; }>;
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
has_double:          1
is_mathy:            1
is_not_stringy:      1
has_int:             1
with_string:         dhagedorn::types::typelist<double, float, int, char, int, char, float, double, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >
with_void:           dhagedorn::types::typelist<void, double, float, int, char, int, char, float, double>
set:                 dhagedorn::types::typelist<double, float, int, char>
no_floats:           dhagedorn::types::typelist<int, char, int, char>
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

