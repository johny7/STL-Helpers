#pragma once

#include <type_traits>

namespace Details_ {
//  tags
constexpr int MAX = 10;
template <int N> struct P : P<N + 1> {};
template <> struct P<MAX> {};

template<template<typename...> class TEMPLATE>
std::false_type is_instance_of_h(P<MAX>, ...);

template<template<typename...> class TEMPLATE, class PARAM1>
std::true_type is_instance_of_h(P<9>, TEMPLATE<PARAM1>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2>
std::true_type is_instance_of_h(P<8>, TEMPLATE<PARAM1, PARAM2>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3>
std::true_type is_instance_of_h(P<7>, TEMPLATE<PARAM1, PARAM2, PARAM3>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4>
std::true_type is_instance_of_h(P<6>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5>
std::true_type is_instance_of_h(P<5>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6>
std::true_type is_instance_of_h(P<4>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6, class PARAM7>
std::true_type is_instance_of_h(P<3>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6, class PARAM7, class PARAM8>
std::true_type is_instance_of_h(P<2>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6, class PARAM7, class PARAM8, class PARAM9>
std::true_type is_instance_of_h(P<1>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8, PARAM9>);

template<template<typename...> class TEMPLATE, class PARAM1, class PARAM2, class PARAM3, class PARAM4, class PARAM5, class PARAM6, class PARAM7, class PARAM8, class PARAM9, class PARAM10>
std::true_type is_instance_of_h(P<0>, TEMPLATE<PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8, PARAM9, PARAM10>);
}

template<template<typename...> class TEMPLATE, typename T>
consteval auto is_instance_of() { return decltype(Details_::is_instance_of_h<TEMPLATE>( Details_::P<0>{}, std::declval<T>() ))(); }

template<typename T, template<typename...> class TEMPLATE>
concept instance_of = is_instance_of<TEMPLATE, T>().value;