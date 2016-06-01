
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_SIGNAL_H_INCLUDED
#define REACT_SIGNAL_H_INCLUDED

#pragma once

#if _MSC_VER && !__INTEL_COMPILER
#pragma warning(disable : 4503)
#endif

#include "react/detail/Defs.h"

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "react/Forward.h"
#include "react/Observer.h"
#include "react/TypeTraits.h"
#include "react/detail/SignalBase.h"

namespace react {

namespace impl {

template <typename... TSignals> struct DomainTrait;

template <typename... TSignals>
using domain_t = typename DomainTrait<std::decay_t<TSignals>...>::type;

template <typename TSignal>
using value_t = typename std::decay_t<TSignal>::value_type;

template <typename T>
using if_not_signal_t = std::enable_if_t<!IsSignal<std::decay_t<T>>::value>;

template <typename T>
using if_signal_t = std::enable_if_t<IsSignal<std::decay_t<T>>::value>;

template <typename T>
using if_not_event_t = std::enable_if_t<!IsEvent<std::decay_t<T>>::value>;

template <typename T>
using if_event_t = std::enable_if_t<IsEvent<std::decay_t<T>>::value>;

template <typename TSignal> struct DomainTrait<TSignal> {
  using type = typename TSignal::DomainT;
};

template <typename TSignal, typename... TSignals>
struct DomainTrait<TSignal, TSignals...> {
  static_assert(std::is_same<typename DomainTrait<TSignal>::type,
                             typename DomainTrait<TSignals...>::type>::value,
                "domain mismatch");

  using type = typename TSignal::DomainT;
};

template <typename... TSignals> struct DomainTrait<SignalPack<TSignals...>> {
  using type = typename DomainTrait<TSignals...>::type;
};

template <typename S, typename F, typename... Args>
using functor_t = FunctionOp<S, F, Args...>;

template <typename D, typename S, typename TOp>
using signal_op_node_t = SignalOpNode<D, S, TOp>;

template <typename D, typename S, typename F, typename... Args>
auto make_op_node(F &&f, Args &&... args) {
  using TOp = functor_t<S, F, std::decay_t<Args>...>;
  return std::make_shared<signal_op_node_t<D, S, TOp>>(
      TOp{std::forward<F>(f), std::forward<Args>(args)...});
}

template <typename F, typename... TSignals>
auto make_op_signal(F &&f, TSignals &&... args) {
  using D = domain_t<TSignals...>;
  using S = std::result_of_t<F(value_t<TSignals>...)>;

  return Signal<D, S>{make_op_node<D, S>(
      std::forward<F>(f), GetNodePtr(std::forward<TSignals>(args))...)};
}

using namespace std::placeholders;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// SignalPack - Wraps several nodes in a tuple. Create with comma operator.
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... TSignals> struct SignalPack {
  SignalPack(const TSignals &... deps) : Data{deps...} {}

  template <typename... TCurSignals, typename TSignal>
  SignalPack(const SignalPack<TCurSignals...> &curArgs, const TSignal &newArg)
      : Data{std::tuple_cat(curArgs.Data, std::tie(newArg))} {}

  std::tuple<const TSignals &...> Data;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// With - Utility function to create a SignalPack
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename... TSignals>
auto With(const TSignals &... some_signals) -> SignalPack<TSignals...> {
  return SignalPack<TSignals...>{some_signals...};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename D, typename V, typename S = typename std::decay<V>::type,
          class = impl::if_not_signal_t<S>, class = impl::if_not_event_t<S>>
auto MakeVar(V &&value) -> VarSignal<D, S> {
  return VarSignal<D, S>(
      std::make_shared<REACT_IMPL::VarNode<D, S>>(std::forward<V>(value)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeVar (higher order reactives)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename V, typename S = typename std::decay<V>::type,
          typename TInner = typename S::value_type,
          class = typename std::enable_if<IsSignal<S>::value>::type>
auto MakeVar(V &&value) -> VarSignal<D, Signal<D, TInner>> {
  return VarSignal<D, Signal<D, TInner>>(
      std::make_shared<REACT_IMPL::VarNode<D, Signal<D, TInner>>>(
          std::forward<V>(value)));
}

template <typename D, typename V, typename S = typename std::decay<V>::type,
          typename TInner = typename S::value_type,
          class = typename std::enable_if<IsEvent<S>::value>::type>
auto MakeVar(V &&value) -> VarSignal<D, Events<D, TInner>> {
  return VarSignal<D, Events<D, TInner>>(
      std::make_shared<REACT_IMPL::VarNode<D, Events<D, TInner>>>(
          std::forward<V>(value)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// MakeSignal
///////////////////////////////////////////////////////////////////////////////////////////////////

// Single arg
template <typename D, typename V, typename FIn, typename F = std::decay_t<FIn>,
          typename S = std::result_of_t<F(V)>>
auto MakeSignal(const Signal<D, V> &arg, FIn &&func) {
  return Signal<D, S>(
      impl::make_op_node<D, S>(std::forward<FIn>(func), GetNodePtr(arg)));
}

// Multiple args
template <typename D, typename S, typename F, typename Tuple, std::size_t... I>
auto MakeNode(F &&f, const Tuple &tuple, std::index_sequence<I...>) {
  return impl::make_op_node<D, S>(std::forward<F>(f),
                                  GetNodePtr(std::get<I>(tuple))...);
}

// Multiple args
template <typename... TSignals, typename FIn,
          typename D = impl::domain_t<TSignals...>,
          typename F = std::decay_t<FIn>,
          typename S = std::result_of_t<F(ValueT<TSignals>...)>>
auto MakeSignal(const SignalPack<TSignals...> &argPack, FIn &&func) {
  return Signal<D, S>{
      MakeNode<D, S>(std::forward<FIn>(func), argPack.Data,
                     impl::indices_t<std::tuple<const TSignals &...>>())};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// ValueTypeTrait
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename S> struct ValueTypeTrait { using type = S; };

template <typename S> struct ValueTypeTrait<S &> {
  using type = std::reference_wrapper<S>;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Signal
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename D, typename S> struct Signal {
  using value_type = typename ValueTypeTrait<S>::type;
  using ValueT = value_type;
  using NodeT = REACT_IMPL::SignalNode<D, value_type>;
  using NodePtrT = std::shared_ptr<NodeT>;
  using DomainT = D;

  Signal() : ptr_{} {}
  explicit Signal(NodePtrT p) : ptr_{std::move(p)} {}

  //  Signal(const Signal &) = delete;
  //  Signal& operator =(const Signal &) = delete;

  //  Signal(Signal &&) = default;
  //  Signal& operator =(Signal &&) = default;

  const value_type &Value() const { return ptr_->ValueRef(); }
  const value_type &operator()() const { return Value(); }

  template <typename R> bool Equals(const Signal<D, R> &other) const {
    return ptr_ == other.ptr_;
  }

  const NodePtrT &node_ptr() const { return ptr_; }

  bool Equals(const Signal &other) const { return ptr_ == other.ptr_; }

protected:
  using input_manager_type = REACT_IMPL::InputManager<D>;

  static input_manager_type &input_manager() {
    return REACT_IMPL::DomainSpecificInputManager<D>::Instance();
  }

private:
  NodePtrT ptr_;
};

template <typename D, typename L, typename R>
bool Equals(const Signal<D, L> &lhs, const Signal<D, R> &rhs) {
  return lhs.Equals(rhs);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Operators
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename D, typename LV, typename RV>
auto operator+(const Signal<D, LV> &l, const Signal<D, RV> &r) {
  return impl::make_op_signal(std::plus<>(), l, r);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<RV>>
auto operator+(const Signal<D, LV> &l, RV &&r) {
  return impl::make_op_signal(
      std::bind(std::plus<>(), impl::_1, std::forward<RV>(r)), l);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<LV>>
auto operator+(LV &&l, const Signal<D, RV> &r) {
  return impl::make_op_signal(
      std::bind(std::plus<>(), std::forward<LV>(l), impl::_1), r);
}

template <typename D, typename LV, typename RV>
auto operator-(const Signal<D, LV> &l, const Signal<D, RV> &r) {
  return impl::make_op_signal(std::minus<>(), l, r);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<RV>>
auto operator-(const Signal<D, LV> &l, RV &&r) {
  return impl::make_op_signal(
      std::bind(std::minus<>(), impl::_1, std::forward<RV>(r)), l);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<LV>>
auto operator-(LV &&l, const Signal<D, RV> &r) {
  return impl::make_op_signal(
      std::bind(std::minus<>(), std::forward<LV>(l), impl::_1), r);
}

template <typename D, typename LV, typename RV>
auto operator*(const Signal<D, LV> &l, const Signal<D, RV> &r) {
  return impl::make_op_signal(std::multiplies<>(), l, r);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<RV>>
auto operator*(const Signal<D, LV> &l, RV &&r) {
  return impl::make_op_signal(
      std::bind(std::multiplies<>(), impl::_1, std::forward<RV>(r)), l);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<LV>>
auto operator*(LV &&l, const Signal<D, RV> &r) {
  return impl::make_op_signal(
      std::bind(std::multiplies<>(), std::forward<LV>(l), impl::_1), r);
}

template <typename D, typename LV, typename RV>
auto operator/(const Signal<D, LV> &l, const Signal<D, RV> &r) {
  return impl::make_op_signal(std::divides<>(), l, r);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<RV>>
auto operator/(const Signal<D, LV> &l, RV &&r) {
  return impl::make_op_signal(
      std::bind(std::divides<>(), impl::_1, std::forward<RV>(r)), l);
}

template <typename D, typename LV, typename RV,
          class = impl::if_not_signal_t<LV>>
auto operator/(LV &&l, const Signal<D, RV> &r) {
  return impl::make_op_signal(
      std::bind(std::divides<>(), std::forward<LV>(l), impl::_1), r);
}

template <typename D, typename S>
Signal<D, S> operator-(const Signal<D, S> &in) {
  return S{-1} * in;
}

template <typename D, typename S>
const auto &GetNodePtr(const Signal<D, S> &s) {
  return s.node_ptr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TInnerSignal,
          typename TInnerValue = typename TInnerSignal::value_type>
auto Flatten(const Signal<D, TInnerSignal> &outer) -> Signal<D, TInnerValue> {
  return Signal<D, TInnerValue>(
      std::make_shared<
          REACT_IMPL::FlattenNode<D, Signal<D, TInnerValue>, TInnerValue>>(
          GetNodePtr(outer), GetNodePtr(outer.Value())));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// VarSignal
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename D, typename S> struct VarSignal : Signal<D, S> {
  using NodeT = REACT_IMPL::VarNode<D, S>;
  using NodePtrT = std::shared_ptr<NodeT>;
  using typename Signal<D, S>::value_type;

  explicit VarSignal(NodePtrT nodePtr) : Signal<D, S>{nodePtr} {}

  void Set(const value_type &newValue) const {
    Signal<D, S>::input_manager().AddInput(var_node(), newValue);
  }

  void Set(value_type &&newValue) const {
    Signal<D, S>::input_manager().AddInput(var_node(), std::move(newValue));
  }

  const VarSignal &operator<<=(const value_type &newValue) const {
    Set(newValue);
    return *this;
  }

  const VarSignal &operator<<=(value_type &&newValue) const {
    Set(std::move(newValue));
    return *this;
  }

  template <typename F> void Modify(const F &func) const {
    Signal<D, S>::input_manager().ModifyInput(var_node(), func);
  }

private:
  NodeT &var_node() const {
    return *std::static_pointer_cast<NodeT>(this->node_ptr());
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to create signal pack from 2 signals.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename TLeftVal, typename TRightVal>
auto operator,(const Signal<D, TLeftVal> &a, const Signal<D, TRightVal> &b) {
  return SignalPack<Signal<D, TLeftVal>, Signal<D, TRightVal>>{a, b};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Comma operator overload to append node to existing signal pack.
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename D, typename... TCurSignals, typename TAppendValue>
auto operator,(const SignalPack<TCurSignals...> &cur,
               const Signal<D, TAppendValue> &append) {
  return SignalPack<TCurSignals..., Signal<D, TAppendValue>>{cur, append};
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// operator->* overload to connect signals to a function and return the
/// resulting signal.
///////////////////////////////////////////////////////////////////////////////////////////////////

// Single arg
template <typename D, typename F, typename TValue>
auto operator->*(const Signal<D, TValue> &arg, F &&func) {
  return MakeSignal(arg, std::forward<F>(func));
}

// Multiple args
template <typename F, typename... TValues>
auto operator->*(const SignalPack<TValues...> &argPack, F &&func) {
  return MakeSignal(argPack, std::forward<F>(func));
}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Flatten macros
///////////////////////////////////////////////////////////////////////////////////////////////////
// Note: Using static_cast rather than -> return type, because when using lambda
// for inline
// class initialization, decltype did not recognize the parameter r
// Note2: MSVC doesn't like typename in the lambda
#if _MSC_VER && !__INTEL_COMPILER
#define REACT_MSVC_NO_TYPENAME
#else
#define REACT_MSVC_NO_TYPENAME typename
#endif

#define REACTIVE_REF(obj, name)                                                \
  Flatten(MakeSignal(                                                          \
      obj, [](const REACT_MSVC_NO_TYPENAME REACT_IMPL::Identity<decltype(      \
                  obj)>::Type::ValueT &r) { return r.name; }))

#define REACTIVE_PTR(obj, name)                                                \
  Flatten(MakeSignal(                                                          \
      obj, [](REACT_MSVC_NO_TYPENAME                                           \
                  REACT_IMPL::Identity<decltype(obj)>::Type::ValueT r) {       \
        assert(r != nullptr);                                                  \
        using T = decltype(r->name);                                           \
        using S = REACT_MSVC_NO_TYPENAME REACT::DecayInput<T>::Type;           \
        return static_cast<S>(r->name);                                        \
      }))

#endif // REACT_SIGNAL_H_INCLUDED
