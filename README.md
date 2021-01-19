# cpp-typelist

A functional-style typelist for C++20.

Lambdas are passed as predicates to algorithms (filter, transform, find_if, etc.)
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

# Small Example
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

# Complete Example

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

