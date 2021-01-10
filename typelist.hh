template<typename ...T>
struct TypeList {
    template<template <typename ...> class TYPE>
    using As = TYPE<T...>;

    template<typename OTHER>
    constexpr static bool contains = (std::is_same_v<OTHER, T> || ...);

    template<typename ...OTHER>
    using Append = TypeList<T..., OTHER...>;

private:
    template<typename LIST>
    static LIST* dedupeImpl() {
        return nullptr;
    }

    template<typename LIST, typename FIRST, typename ...REST>
    static auto* dedupeImpl() {
        if constexpr (LIST::template contains<FIRST>) {
            return dedupeImpl<LIST, REST...>() ;
        } else {
            using NEW = typename LIST::template Append<FIRST>;
            return dedupeImpl<NEW, REST...>();
        }
    };

public:
    using Dedupe = std::remove_pointer_t<decltype(dedupeImpl<TypeList<>, T...>())>;
};
