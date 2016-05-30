
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_UTIL_H_INCLUDED
#define REACT_COMMON_UTIL_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <tuple>
#include <type_traits>
#include <utility>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Unpack tuple - see
/// http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments
///////////////////////////////////////////////////////////////////////////////////////////////////

template<typename F, typename Tuple, std::size_t ... I>
auto apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>) {
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

template <typename Tuple>
using indices_t = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;


template<typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) {
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), indices_t<Tuple>());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper to enable calling a function on each element of an argument pack.
/// We can't do f(args) ...; because ... expands with a comma.
/// But we can do nop_func(f(args) ...);
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename... TArgs>
inline void pass(TArgs&& ...) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Identity (workaround to enable enable decltype()::X)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Identity
{
    using Type = T;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DontMove!
///////////////////////////////////////////////////////////////////////////////////////////////////
struct DontMove {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DisableIfSame
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename U>
struct DisableIfSame :
    std::enable_if<! std::is_same<
        typename std::decay<T>::type,
        typename std::decay<U>::type>::value> {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddDummyArgWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TArg, typename F, typename TRet, typename ... TDepValues>
struct AddDummyArgWrapper
{
    AddDummyArgWrapper(const AddDummyArgWrapper& other) = default;

    AddDummyArgWrapper(AddDummyArgWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template <typename FIn, class = typename DisableIfSame<FIn,AddDummyArgWrapper>::type>
    explicit AddDummyArgWrapper(FIn&& func) : MyFunc( std::forward<FIn>(func) ) {}

    TRet operator()(TArg, TDepValues& ... args)
    {
        return MyFunc(args ...);
    }

    F MyFunc;
};

template <typename TArg, typename F, typename ... TDepValues>
struct AddDummyArgWrapper<TArg,F,void,TDepValues...>
{
public:
    AddDummyArgWrapper(const AddDummyArgWrapper& other) = default;

    AddDummyArgWrapper(AddDummyArgWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template <typename FIn, class = typename DisableIfSame<FIn,AddDummyArgWrapper>::type>
    explicit AddDummyArgWrapper(FIn&& func) : MyFunc( std::forward<FIn>(func) ) {}

    void operator()(TArg, TDepValues& ... args)
    {
        MyFunc(args ...);
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddDefaultReturnValueWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template 
<
    typename F,
    typename TRet,
    TRet return_value
>
struct AddDefaultReturnValueWrapper
{
    AddDefaultReturnValueWrapper(const AddDefaultReturnValueWrapper&) = default;

    AddDefaultReturnValueWrapper(AddDefaultReturnValueWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddDefaultReturnValueWrapper>::type
    >
    explicit AddDefaultReturnValueWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    template <typename ... TArgs>
    TRet operator()(TArgs&& ... args)
    {
        MyFunc(std::forward<TArgs>(args) ...);
        return return_value;
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsCallableWith
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename F,
    typename TRet,
    typename ... TArgs
>
class IsCallableWith
{
private:
    using NoT = char[1];
    using YesT = char[2];

    template
    <
        typename U,
        class = decltype(
            static_cast<TRet>(
                (std::declval<U>())(std::declval<TArgs>() ...)))
    >
    static YesT& check(void*);

    template <typename U>
    static NoT& check(...);

public:
    enum { value = sizeof(check<F>(nullptr)) == sizeof(YesT) };
};

/****************************************/ REACT_IMPL_END /***************************************/

// Expand args by wrapping them in a dummy function
// Use comma operator to replace potential void return value with 0
#define REACT_EXPAND_PACK(...) REACT_IMPL::pass((__VA_ARGS__ , 0) ...)

#endif // REACT_COMMON_UTIL_H_INCLUDED
