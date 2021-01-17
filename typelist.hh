#include <type_traits>
#include <cxxabi.h>
#include <utility>
#include <array>
#include <memory>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <variant>

using no_type = struct no_type_t;

template<typename T>
static constexpr auto false_type_v = false; 

template<typename ...T>
struct TypeList {
    constexpr static std::size_t size = sizeof...(T);

    constexpr static auto indices = std::make_index_sequence<size>();

    template<template <typename ...> class TYPE>
    using As = TYPE<T...>;

    template<typename OTHER>
    constexpr static bool contains = (std::is_same_v<OTHER, T> || ...);

    template<typename ...OTHER>
    using append = TypeList<T..., OTHER...>;

    template<template <typename ...OTHER> class TypeList, typename ...OTHER>
    using concat = TypeList<T..., OTHER...>;

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
            using NEW = typename LIST::template append<FIRST>;
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
            PREDICATE.template operator()<INDICES, T>()...
        };

        return result;
    }

    template<auto PREDICATE, auto ...INDICES>
    static constexpr auto* mapTImpl(std::index_sequence<INDICES...>) {
        return static_cast<TypeList<
            decltype(PREDICATE.template operator()<INDICES, T>())...
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
                    return static_cast<TypeList<LIST..., FIRST, SECOND>*>(nullptr);
                } else {
                    return sortImpl2<PREDICATE, true, LIST..., FIRST, SECOND>(holder<>{});
                }
            } else {
                if constexpr (NO_SWAP) {
                    return static_cast<TypeList<LIST..., SECOND, FIRST>*>(nullptr);
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
        return static_cast<TypeList<OTHER...>*>(nullptr);
    }
    
public:
    template<auto PREDICATE>
    using filter = std::remove_pointer_t<decltype(filterImpl<PREDICATE, 0, TypeList<>, T...>())>;

    // calling filter<...> directly crashes gcc trunk regardless of how template<> using filter is defined
    // (remove_reference, remove_pointer, just using decltype and returning the pointer-to-typelist type...)
    // only when this using alias is templated
    // with the below workaround instead,
    // using std::remove_pointer_t or std::remove_pointer<...>::value crashes gcc trunk
    // but deref'ing the return of filterImpl and then stripping its reference with std::remove_reference_t seems
    // to be OK...
    template<std::size_t FROM, std::size_t TO>
    using slice = std::remove_reference_t<decltype(*filterImpl<[]<typename TYPE, auto I, typename LIST>(){ return FROM <= I && I < TO; }, 0, TypeList<>, T...>())>;    

    // using's without template arguments are OK to use other templated using aliases - these don't crash GCC
    using dedupe = filter<[]<typename IT, std::size_t I, typename LIST>(){ 
        return !LIST::template contains<IT>;     
    }>;

    template<auto PREDICATE>
    requires requires { PREDICATE.template operator()<0, int>(); }
    using find = std::remove_pointer_t<decltype(findImpl<PREDICATE, 0, T...>())>;

    template<std::size_t INDEX>
    requires (INDEX < size)
    using at = std::remove_reference_t<decltype(*findImpl<[]<auto I, typename LIST>(){ return I == INDEX; }, 0, T...>())>;

    template<auto PREDICATE>
    static constexpr auto map = mapImpl<PREDICATE>(indices);

    template<auto PREDICATE>
    using map_t = std::remove_reference_t<decltype(*mapTImpl<PREDICATE>(indices))>;   

    template<auto PREDICATE = []<typename A, typename B>() { return sizeof(A) < sizeof(B); }>
    using sort = std::remove_reference_t<decltype(*sortImpl2<PREDICATE, false, T...>(holder<>{}))>; 

    template<template <typename ...> class AS>
    using as = AS<T...>;

    template<typename TYPE>
    using from = std::remove_reference_t<decltype(*fromImpl(static_cast<TYPE*>(nullptr)))>;
};

int main() {
    using list = TypeList<int[1], int[2], int[3], char, char, int, int, float>;
    using sorted = list::sort<>;
    using indexed = sorted::at<0>;
    constexpr auto sizes = sorted::map<[]<auto I, typename T>(){ return sizeof(T); }>;
    using variant = sorted::as<std::variant>;
    using from = TypeList<>::from<variant>;

    auto printType = []<typename T>() {
        std::unique_ptr<char> str{abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr)};
        return std::string{str.get()};
    };

    fmt::print("begin\n");
    fmt::print("sizes: {}\n", sizes);
    fmt::print("list: {}\n", printType.operator()<list>());
    fmt::print("sorted list: {}\n", printType.operator()<sorted>());
    fmt::print("sorted[0]: {}\n", printType.operator()<indexed>());
    fmt::print("variant<sorted>: {}\n", printType.operator()<variant>());
    fmt::print("from<variant>: {}\n", printType.operator()<from>());
}

