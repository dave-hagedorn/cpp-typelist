#pragma once

#include <type_traits>
#include <utility>
#include <array>
#include <concepts>

namespace dhagedorn::types {

template<auto PREDICATE>
concept is_complex_iter_predicate = requires {
    { PREDICATE.template operator()<int, std::size_t{0}>() } -> std::convertible_to<bool>;
};

template<auto PREDICATE>
concept is_simple_iter_predicate = requires {
    { PREDICATE.template operator()<int>() } -> std::convertible_to<bool>;
};

template<auto PREDICATE>
concept is_iter_predicate = is_simple_iter_predicate<PREDICATE> || is_complex_iter_predicate<PREDICATE>;

template<typename ...TYPES>
struct type_list;

template<auto PREDICATE>
concept is_complex_filter_predicate = requires {
    { PREDICATE.template operator()<int, std::size_t{0}, type_list<>>() } -> std::convertible_to<bool>;
};

template<auto PREDICATE>
concept is_simple_filter_predicate = requires {
    { PREDICATE.template operator()<int>() } -> std::convertible_to<bool>;
};

template<auto PREDICATE>
concept is_filter_predicate = is_simple_filter_predicate<PREDICATE> || is_complex_filter_predicate<PREDICATE>;

template<auto PREDICATE>
concept is_comp_predicate = requires {
    { PREDICATE.template operator()<int, int>() } -> std::convertible_to<bool>;
};

template<typename T>
struct is_typelist_t: std::false_type {};

template<typename ...TYPES>
struct is_typelist_t<type_list<TYPES...>>: std::true_type {};

template<typename T>
concept is_typelist = is_typelist_t<T>::value;


/**
 * Invalid type - used to indicate an empty/non-existent type
 * Ex - a type_list<>::find call that does not find a matching type
 */
struct no_type {};

/**
 * Wrap a set of types into a dependant false value, for use with static_assert()'s
 * in constexpr if's
 */
template<typename ...TYPES>
static constexpr auto false_type_v = false;

/**
 * Holds a list of types.  Can perform various compile time computations, generating new
 * type lists or constexpr values
 */
template<typename ...TYPES>
struct type_list {
private:
template<typename ...OTHER>
    struct holder {

    };

        template<typename OTHER>
    constexpr static auto contains_one = ((std::is_same_v<OTHER, TYPES>) || ...);

    template<auto PREDICATE, typename T, auto I>
    constexpr static auto invoke_iter_predicate() {
        if constexpr (is_simple_iter_predicate<PREDICATE>) {
            return PREDICATE.template operator()<T>();
        } else {
            return PREDICATE.template operator()<T, I>();
        }
    }

    template<auto PREDICATE, typename T, auto I, is_typelist LIST>
    constexpr static auto invoke_filter_predicate() {
        if constexpr (is_simple_filter_predicate<PREDICATE>) {
            return PREDICATE.template operator()<T>();
        } else {
            return PREDICATE.template operator()<T, I, LIST>();
        }
    }    

    template<auto PREDICATE, auto I, is_typelist LIST>
    static LIST* filter_impl() {
        return nullptr;
    }

    template<auto PREDICATE, auto I, is_typelist LIST, typename FIRST, typename ...REST>
    static auto* filter_impl() {
        constexpr auto PASS = invoke_filter_predicate<PREDICATE, FIRST, I, LIST>();

        if constexpr (PASS) {
            using NEW_LIST = typename LIST::template push_back<FIRST>;
            if constexpr (is_filter_predicate<PREDICATE>) {
                return filter_impl<PREDICATE, I+1, NEW_LIST, REST...>();   
            } else {
                return filter_impl<PREDICATE, I+1, NEW_LIST, REST...>();   
            }
        } else {
            return filter_impl<PREDICATE, I+1, LIST, REST...>();
        }
    };

    template<auto PREDICATE, auto I>
    static no_type* find_if_impl() {
        return nullptr;
    }    

    template<auto PREDICATE, auto I, typename FIRST, typename ...REST>
    static auto* find_if_impl() {
        constexpr auto PASS = invoke_iter_predicate<PREDICATE, FIRST, I>;

        if constexpr (PASS) {
            return static_cast<FIRST*>(nullptr);
        } else {
            return find_if_impl<PREDICATE, I+1, REST...>();
        }
    }

    template<auto PREDICATE, auto ...INDICES>
    static constexpr auto transform_v_impl(std::index_sequence<INDICES...>) {
        std::array result = {
            invoke_iter_predicate<PREDICATE, TYPES, INDICES>()...
        };

        return result;
    }

    template<auto PREDICATE, auto ...INDICES>
    static constexpr auto* transform_impl(std::index_sequence<INDICES...>) {
        return static_cast<type_list<
            decltype(invoke_iter_predicate<PREDICATE, TYPES, INDICES>())...
        >*>(nullptr);
    }

    template<auto PREDICATE, bool NO_SWAP, typename FIRST, typename SECOND, typename ...REST, typename ...LIST>
    static constexpr auto* sortImpl2(holder<LIST...>) {
        constexpr bool LESS_THAN = PREDICATE.template operator()<FIRST, SECOND>();
        constexpr bool EQUAL = !PREDICATE.template operator()<FIRST, SECOND>() && !PREDICATE.template operator()<SECOND, FIRST>();
        constexpr bool LESS_THAN_OR_EQUAL = LESS_THAN || EQUAL;
        constexpr bool DONE_PASS = sizeof...(REST) == 0;

        if constexpr (DONE_PASS) {
            if constexpr (LESS_THAN_OR_EQUAL) {
                if constexpr (NO_SWAP) {
                    return static_cast<type_list<LIST..., FIRST, SECOND>*>(nullptr);
                } else {
                    return sortImpl2<PREDICATE, true, LIST..., FIRST, SECOND>(holder<>{});
                }
            } else {
                if constexpr (NO_SWAP) {
                    return static_cast<type_list<LIST..., SECOND, FIRST>*>(nullptr);
                } else {
                    return sortImpl2<PREDICATE, true, LIST..., SECOND, FIRST>(holder<>{});
                }
            }
        } else if constexpr (LESS_THAN_OR_EQUAL) {
            return sortImpl2<PREDICATE, NO_SWAP && true, SECOND, REST...>(holder<LIST..., FIRST>{});
        } else {
            return sortImpl2<PREDICATE, NO_SWAP && false, FIRST, REST...>(holder<LIST..., SECOND>{});
        }
    }

    template<template <typename ...> class FROM, typename ...OTHER>
    constexpr static auto* fromImpl(FROM<OTHER...>*) {
        return static_cast<type_list<OTHER...>*>(nullptr);
    }

public:
    /**
     * Note - the weird form of std::remove_reference_t<decltype(*some_operation(...))>
     * seems to work well on gcc 10.x
     * The simpler std::remove_pointer_t<decltype(some_operation())>
     * can crash the compiler if one using directive wraps another
     * This seems to happen only for templated using directives with non-type-template parameters
     */

    /**
     * Size of this typelist - number of types it contains
     */
    constexpr static std::size_t size = sizeof...(TYPES);

    /**
     * std::index_sequence of the indices for this type list
     * Used internally, but may have use by consuers as well
     */
    constexpr static auto indices = std::make_index_sequence<size>();

    /**
     * Evaluates to true if PREDICATE returns true for any type in this list
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    constexpr static bool any_of = []<auto ...I>(std::index_sequence<I...>){ 
        return (invoke_iter_predicate<PREDICATE, TYPES, I>() || ...);
    }(indices);

    /**
     * Evaluates to true if PREDICATE returns true for all types in this list
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    constexpr static bool all_of = []<auto ...I>(std::index_sequence<I...>){ 
        return (invoke_iter_predicate<PREDICATE, TYPES, I>() || ...);
    }(indices);

    /**
     * Opposite of any_of
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    constexpr static bool none_of = !any_of<PREDICATE>;

    /**
     * Evaluates to true if this type_list contains all of the types in OTHER
     */
    template<typename ...OTHER>
    constexpr static auto contains = ((contains_one<OTHER> )&& ...); 

    /**
     * Append the provided types to this list, returning a new type_list
     */
    template<typename ...OTHER>
    using push_back = type_list<TYPES..., OTHER...>;

    /**
     * Prepend the provided types to this list, returning a new type_list
     */
    template<typename ...OTHER>
    using push_front = type_list<OTHER..., TYPES...>;

    /**
     * Generate a new type_list by filtering the types in this list using the provided
     * predicate PREDICATE<T>() or PREDICATE<T, INDEX, CURRENT_TYPE_LIST>()
     */
    template<auto PREDICATE>
    requires is_filter_predicate<PREDICATE>
    using filter = std::remove_reference_t<decltype(*filter_impl<PREDICATE, 0, type_list<>, TYPES...>())>;

    /**
     * Generate a new type_list that is a set of this type_list - that is, no duplicate entries
     * note:  This form of using-wrapping-another-using seems to be OK (doesn't crash gcc).  The
     *        difference seems to be the top-level using expression doesn't have template arguments in this case
     */
    using set = filter<[]<typename IT, auto I, is_typelist LIST>(){
        return !LIST::template contains<IT>;
    }>;

    /**
     * Generate a new type_list by taking the types with indices [FROM,TO)
     * (typical slice operation)
     * note:  The bizarre unwrapping below is because remove_pointer_t crashes on gcc trunk, and
     *        calling directly as `filter<PREDICATE>;` also crashes gcc trunk
     */
    template<std::size_t FROM, std::size_t TO>
    requires (FROM <= size && TO <= size && FROM < TO)
    using slice = std::remove_reference_t<decltype(*filter_impl<[]<typename TYPE, auto I, typename LIST>(){ return FROM <= I && I < TO; }, 0, type_list<>, TYPES...>())>;    

    /**
     * Return the first type matching the provided PREDICATE
     * If no type in this list matches, returns `no_type`
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    using find_if = std::remove_pointer_t<decltype(find_if_impl<PREDICATE, 0, TYPES...>())>;

    /**
     * Returns the type at the specified index
     */
    template<std::size_t INDEX>
    requires (INDEX < size)
    using at = std::remove_reference_t<decltype(*find_if_impl<[]<typename T, auto I>(){ return I == INDEX; }, 0, TYPES...>())>;

    /**
     * Transform this type_list into a std::array by converting its
     * types into values using PREDICATE
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    static constexpr auto transform_v = transform_v_impl<PREDICATE>(indices);

    /**
     * Transform this type_list into another type_list using the return type
     * of the provided PREDICATE as a guide
     */
    template<auto PREDICATE>
    requires is_iter_predicate<PREDICATE>
    using transform_with = std::remove_reference_t<decltype(*transform_impl<PREDICATE>(indices))>;

    /**
     * Transform this type_list into another type_list using the provided type as a guide
     */
    template<template <typename> class FROM>
    using transform = type_list<FROM<TYPES>...>;  

    /**
     * Stable sort - ascending
     * PREDICATE must return `true` if A < B (strict weak ordering)
     * defaults to sizeof(A) < sizeof(B)
     */
    template<auto PREDICATE = []<typename A, typename B>() { return sizeof(A) < sizeof(B); }>
    requires is_comp_predicate<PREDICATE>
    using sort = std::remove_reference_t<decltype(*sortImpl2<PREDICATE, false, TYPES...>(holder<>{}))>;

    /**
     * Inject the types in this list into the provided template class
     * ex:  type_list::as<std::variant>;
     */
    template<template <typename ...> class AS>
    using as = AS<TYPES...>;

    /**
     * Extrat the types from the provided template type, injecting
     * them into a new type_list
     */
    template<typename TYPE>
    using from = std::remove_reference_t<decltype(*fromImpl(static_cast<TYPE*>(nullptr)))>;
};

}


#include <memory>
#include <cxxabi.h>
#include <variant>
#include <iostream>
#include <sstream>
#include <cxxabi.h>

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

}
