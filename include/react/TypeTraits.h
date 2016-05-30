
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_TYPETRAITS_H_INCLUDED
#define REACT_TYPETRAITS_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include "react/Forward.h"

/*****************************************/ REACT_BEGIN /*****************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsSignal
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsSignal { static const bool value = false; };

template <typename D, typename T>
struct IsSignal<Signal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsSignal<VarSignal<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsSignal<OpSignal<D,T,TOp>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsEvent
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsEvent { static const bool value = false; };

template <typename D, typename T>
struct IsEvent<Events<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsEvent<EventSource<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsEvent<TempEvents<D,T,TOp>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsObserver
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsObserver { static const bool value = false; };

template <typename D>
struct IsObserver<Observer<D>> { static const bool value = true; };

template <typename D>
struct IsObserver<ScopedObserver<D>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsContinuation
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsContinuation { static const bool value = false; };

template <typename D, typename D2>
struct IsContinuation<Continuation<D,D2>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsObservable
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsObservable { static const bool value = false; };

template <typename D, typename T>
struct IsObservable<Signal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsObservable<VarSignal<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsObservable<OpSignal<D,T,TOp>> { static const bool value = true; };

template <typename D, typename T>
struct IsObservable<Events<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsObservable<EventSource<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsObservable<TempEvents<D,T,TOp>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsReactive
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct IsReactive { static const bool value = false; };

template <typename D, typename T>
struct IsReactive<Signal<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<VarSignal<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsReactive<OpSignal<D,T,TOp>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<Events<D,T>> { static const bool value = true; };

template <typename D, typename T>
struct IsReactive<EventSource<D,T>> { static const bool value = true; };

template <typename D, typename T, typename TOp>
struct IsReactive<TempEvents<D,T,TOp>> { static const bool value = true; };

template <typename D>
struct IsReactive<Observer<D>> { static const bool value = true; };

template <typename D>
struct IsReactive<ScopedObserver<D>> { static const bool value = true; };

template <typename D, typename D2>
struct IsReactive<Continuation<D,D2>> { static const bool value = true; };

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DecayInput
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct DecayInput { using Type = T; };

template <typename D, typename T>
struct DecayInput<VarSignal<D,T>> { using Type = Signal<D,T>; };

template <typename D, typename T>
struct DecayInput<EventSource<D,T>> { using Type = Events<D,T>; };

/******************************************/ REACT_END /******************************************/

#endif // REACT_TYPETRAITS_H_INCLUDED
