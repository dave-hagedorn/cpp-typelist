//#pragma once

#include <type_traits>
#include <utility>
#include <array>

namespace dhagedorn::types {

/**
 * Invalid type - used to indicate an empty/non-existent type
 * Ex - a type_list<>::find call that does not find a matching type
 */
using no_type = struct no_type_t;

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

    template<auto PREDICATE, auto I, typename LIST>
    static LIST* filterImpl() {
        return nullptr;
    }

    template<auto PREDICATE, auto I, typename LIST, typename FIRST, typename ...REST>
    static auto* filterImpl() {
        if constexpr (PREDICATE.template operator()<FIRST, I, LIST>()) {
            using NEW = typename LIST::template push_back<FIRST>;
            return filterImpl<PREDICATE, I+1, NEW, REST...>();
        } else {
            return filterImpl<PREDICATE, I+1, LIST, REST...>();
        }
    };

    template<auto PREDICATE, auto I, typename FIRST, typename ...REST>
    static auto* findImpl() {
        if constexpr (PREDICATE.template operator()<I, FIRST>()) {
            return static_cast<FIRST*>(nullptr);
        } else if constexpr (sizeof...(REST) == 0) {
            return static_cast<no_type*>(nullptr);
        } else {
            return findImpl<PREDICATE, I+1, REST...>();
        }
    }

    template<auto I, auto INDEX, typename FIRST, typename ...REST>
    static auto* atImpl() {
        if constexpr (INDEX == I) {
            return static_cast<FIRST*>(nullptr);
        } else if constexpr (sizeof...(REST) == 0) {
            static_assert(false_type_v<REST...>, "index out of range");
        } else {
            return atImpl<I+1, INDEX, REST...>();
        }
    }

    template<auto PREDICATE, auto ...INDICES>
    static constexpr auto mapImpl(std::index_sequence<INDICES...>) {
        std::array result = {
            PREDICATE.template operator()<INDICES, TYPES>()...
        };

        return result;
    }

    template<auto PREDICATE, auto ...INDICES>
    static constexpr auto* mapTImpl(std::index_sequence<INDICES...>) {
        return static_cast<type_list<
            decltype(PREDICATE.template operator()<INDICES, TYPES>())...
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


    template<typename OTHER>
    constexpr static bool contains_one = (std::is_same_v<OTHER, TYPES> || ...);
public:
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
     * Evaluates to true if this list contains any of the specified types
     */
    template<typename ...OTHER>
    constexpr static bool any_of = (contains_one<OTHER> || ...);

    /**
     * Evaluates to true if this list contains all of the specified types
     */
    template<typename ...OTHER>
    constexpr static bool all_of = (contains_one<OTHER> && ...);    

    /**
     * Append the provided types to this list
     */
    template<typename ...OTHER>
    using push_back = type_list<TYPES..., OTHER...>;

    /**
     * Prepend the provided types to this list
     */
    template<typename ...OTHER>
    using push_front = type_list<OTHER..., TYPES...>;

    /**
     * Generate a new type_list by filtering the types in this list using the provided
     * predicate
     * PREDICATE must have operator()<...>(); or <...>():
     */
    template<auto PREDICATE>
    using filter = std::remove_pointer_t<decltype(filterImpl<PREDICATE, 0, type_list<>, TYPES...>())>;

    // calling filter<...> directly crashes gcc trunk regardless of how template<> using filter is defined
    // (remove_reference, remove_pointer, just using decltype and returning the pointer-to-type_list type...)
    // only when this using alias is templated
    // with the below workaround instead,
    // using std::remove_pointer_t or std::remove_pointer<...>::value crashes gcc trunk
    // but deref'ing the return of filterImpl and then stripping its reference with std::remove_reference_t seems
    // to be OK...
    template<std::size_t FROM, std::size_t TO>
    using slice = std::remove_reference_t<decltype(*filterImpl<[]<typename TYPE, auto I, typename LIST>(){ return FROM <= I && I < TO; }, 0, type_list<>, TYPES...>())>;

    // using's without template arguments are OK to use other templated using aliases - these don't crash GCC
    using unique = filter<[]<typename IT, std::size_t I, typename LIST>(){
        return !LIST::template any_of<IT>;
    }>;

    template<auto PREDICATE>
    requires requires { PREDICATE.template operator()<0, int>(); }
    using find_if = std::remove_pointer_t<decltype(findImpl<PREDICATE, 0, TYPES...>())>;

    template<std::size_t INDEX>
    requires (INDEX < size)
    using at = std::remove_reference_t<decltype(*findImpl<[]<auto I, typename LIST>(){ return I == INDEX; }, 0, TYPES...>())>;

    template<auto PREDICATE>
    static constexpr auto transform_v = mapImpl<PREDICATE>(indices);

    template<auto PREDICATE>
    using transform = std::remove_reference_t<decltype(*mapTImpl<PREDICATE>(indices))>;

    template<auto PREDICATE = []<typename A, typename B>() { return sizeof(A) < sizeof(B); }>
    requires requires { static_cast<bool>(PREDICATE.template operator()<int, int>()); }
    using sort = std::remove_reference_t<decltype(*sortImpl2<PREDICATE, false, TYPES...>(holder<>{}))>;

    template<template <typename ...> class AS>
    using as = AS<TYPES...>;

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
    using list = types::type_list<int[1], int[2], int[3], char, char, int, int, float>;
    using sorted = list::sort<>;
    using indexed = sorted::at<0>;
    constexpr auto sizes = sorted::transform_v<[]<auto I, typename T>(){ return sizeof(T); }>;
    using variant = sorted::as<std::variant>;
    using from = types::type_list<>::from<variant>;

    print("list: ", typeid(list));
    print("sorted by size: ", typeid(sorted));
    print("sizes of types in sorted: ", sizes);
    print("sorted[0]: ", typeid(indexed));
    print("into variant: ", typeid(variant));
    print("from variant: ", typeid(from));
}
