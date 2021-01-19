#include <memory>
#include <cxxabi.h>
#include <variant>
#include <iostream>
#include <sstream>
#include <cxxabi.h>

#include "typelist.hh"

auto format(const auto& anything) {
    return anything;
}

auto format(const std::type_info& tinfo) {
    auto demangled = abi::__cxa_demangle(tinfo.name(), nullptr, nullptr, nullptr);
    std::string out = demangled;
    free(demangled);
    return out;
}

template<typename CONT>
requires (requires (CONT x) { std::cout << format(*x.begin()) << format(*x.end()); })
auto format(const CONT& cont) {
    std::stringstream out;

    for (const auto& e: cont) {
        out << format(e) << ", ";
    }

    out.seekp(-2, std::ios_base::end);
    out << '\0';
    return out.str();
}

template<typename ...T>
requires (requires (T ...x) { ((std::cout << format(x)) , ...); })
void print(const T& ...things) {
    auto printer = [](const auto& thing) {
        std::cout << format(thing);
    };

    ((printer(things)) , ...);
    std::cout << std::endl;

}

namespace types = dhagedorn::types;

int main() {
    using list = types::type_list<double, float, int, char, int, char, float, double>;
    constexpr auto size = list::size;
    constexpr auto has_double = list::any_of<[]<typename T>(){ return std::is_same_v<T,double>; }>;
    constexpr auto is_mathy = list::any_of<[]<typename T>(){ return std::is_integral_v<T> || std::is_floating_point_v<T>; }>;
    constexpr auto is_not_stringy = list::none_of<[]<typename T>(){ return std::is_same_v<T, std::string>; }>;
    constexpr auto has_int = list::contains<int>;
    using with_string = list::push_back<std::string>;
    using with_void = list::push_front<void>;
    using set = list::set;
    using no_floats = list::filter<[]<typename T>(){ return !std::is_floating_point_v<T>; }>;
    using sliced = list::slice<0,3>;
    using first_integral = list::find_if<[]<typename T>(){ return std::is_integral_v<T>; }>;
    using first_type = list::at<0>;
    constexpr auto sizes = list::transform_v<[]<typename T>(){ return sizeof(T); }>;
    using pointy = list::transform_with<[]<typename T>() -> T* { return nullptr; }>;
    using safe_pointy = list::transform<std::shared_ptr>;
    using sorted = list::sort<>;
    using sorted_backwards = list::sort<[]<typename A, typename B>(){ return sizeof(B) < sizeof(A); }>;
    using variant = list::as<std::variant>;
    using from_variant = list::from<variant>;


    print("list:                ", typeid(list));
    print("size:                ", size);
    print("has_double:          ", has_double);
    print("is_mathy:            ", is_mathy);
    print("is_not_stringy:      ", is_not_stringy);
    print("has_int:             ", has_int);
    print("with_string:         ", typeid(with_string));
    print("with_void:           ", typeid(with_void));
    print("set:                 ", typeid(set));
    print("no_floats:           ", typeid(no_floats));
    print("sliced:              ", typeid(sliced));
    print("first_integral:      ", typeid(first_integral));
    print("first_type:          ", typeid(first_type));
    print("sizes:               ", sizes);
    print("pointy:              ", typeid(pointy));
    print("safe_pointy:         ", typeid(safe_pointy));
    print("sorted:              ", typeid(sorted));
    print("sorted_backwards:    ", typeid(sorted_backwards));
    print("variant:             ", typeid(variant));
    print("from_variant:        ", typeid(from_variant));

    return 0;
}
