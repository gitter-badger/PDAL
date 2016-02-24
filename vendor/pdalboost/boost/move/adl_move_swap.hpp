//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2007, 2008 Steven Watanabe, Joseph Gauterin, Niels Dekker
// (C) Copyright Ion Gaztanaga 2005-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_MOVE_ADL_MOVE_SWAP_HPP
#define BOOST_MOVE_ADL_MOVE_SWAP_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

//Based on Boost.Core's swap.
//Many thanks to Steven Watanabe, Joseph Gauterin and Niels Dekker.

#include <boost/config.hpp>
#include <cstddef> //for std::size_t

//Try to avoid including <algorithm>, as it's quite big
#if defined(_MSC_VER) && defined(BOOST_DINKUMWARE_STDLIB)
   #include <utility>   //Dinkum libraries define std::swap in utility which is lighter than algorithm
#elif defined(BOOST_GNU_STDLIB)
   //For non-GCC compilers, where GNUC version is not very reliable, or old GCC versions
   //use the good old stl_algobase header, which is quite lightweight
   #if !defined(BOOST_GCC) || ((__GNUC__ < 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ < 3)))
      #include <bits/stl_algobase.h>
   #elif (__GNUC__ == 4) && (__GNUC_MINOR__ == 3)
      //In GCC 4.3 a tiny stl_move.h was created with swap and move utilities
      #include <bits/stl_move.h>
   #else
      //In GCC 4.4 stl_move.h was renamed to move.h
      #include <bits/move.h>
   #endif
#elif defined(_LIBCPP_VERSION)
   #include <type_traits>  //The initial import of libc++ defines std::swap and still there
#elif __cplusplus >= 201103L
   #include <utility>    //Fallback for C++ >= 2011
#else
   #include <algorithm>  //Fallback for C++98/03
#endif

#include <boost/move/utility_core.hpp> //for pdalboost::move

#if !defined(BOOST_MOVE_DOXYGEN_INVOKED)

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
namespace pdalboost_move_member_swap {

struct dont_care
{
   dont_care(...);
};

struct private_type
{
   static private_type p;
   private_type const &operator,(int) const;
};

typedef char yes_type;            
struct no_type{ char dummy[2]; }; 

template<typename T>
no_type is_private_type(T const &);

yes_type is_private_type(private_type const &);

template <typename Type>
class has_member_function_named_swap
{
   struct BaseMixin
   {
      void swap();
   };

   struct Base : public Type, public BaseMixin { Base(); };
   template <typename T, T t> class Helper{};

   template <typename U>
   static no_type deduce(U*, Helper<void (BaseMixin::*)(), &U::swap>* = 0);
   static yes_type deduce(...);

   public:
   static const bool value = sizeof(yes_type) == sizeof(deduce((Base*)(0)));
};

template<typename Fun, bool HasFunc>
struct has_member_swap_impl
{
   static const bool value = false;
};

template<typename Fun>
struct has_member_swap_impl<Fun, true>
{
   struct FunWrap : Fun
   {
      FunWrap();

      using Fun::swap;
      private_type swap(dont_care) const;
   };

   static Fun &declval_fun();
   static FunWrap declval_wrap();

   static bool const value =
      sizeof(no_type) == sizeof(is_private_type( (declval_wrap().swap(declval_fun()), 0)) );
};

template<typename Fun>
struct has_member_swap : public has_member_swap_impl
      <Fun, has_member_function_named_swap<Fun>::value>
{};

}  //namespace pdalboost_move_member_swap

namespace pdalboost_move_adl_swap{

template<class P1, class P2, bool = P1::value>
struct and_op_impl
{  static const bool value = false; };

template<class P1, class P2>
struct and_op_impl<P1, P2, true>
{  static const bool value = P2::value;   };

template<class P1, class P2>
struct and_op
   : and_op_impl<P1, P2>
{};

//////

template<class P1, class P2, bool = P1::value>
struct and_op_not_impl
{  static const bool value = false; };

template<class P1, class P2>
struct and_op_not_impl<P1, P2, true>
{  static const bool value = !P2::value;   };

template<class P1, class P2>
struct and_op_not
   : and_op_not_impl<P1, P2>
{};

template<class T>
void swap_proxy(T& x, T& y, typename pdalboost::move_detail::enable_if_c<!pdalboost::move_detail::has_move_emulation_enabled_impl<T>::value>::type* = 0)
{
   //use std::swap if argument dependent lookup fails
   //Use using directive ("using namespace xxx;") instead as some older compilers
   //don't do ADL with using declarations ("using ns::func;").
   using namespace std;
   swap(x, y);
}

template<class T>
void swap_proxy(T& x, T& y
               , typename pdalboost::move_detail::enable_if< and_op_not_impl<pdalboost::move_detail::has_move_emulation_enabled_impl<T>
                                                                        , pdalboost_move_member_swap::has_member_swap<T> >
                                                       >::type* = 0)
{  T t(::pdalboost::move(x)); x = ::pdalboost::move(y); y = ::pdalboost::move(t);  }

template<class T>
void swap_proxy(T& x, T& y
               , typename pdalboost::move_detail::enable_if< and_op_impl< pdalboost::move_detail::has_move_emulation_enabled_impl<T>
                                                                    , pdalboost_move_member_swap::has_member_swap<T> >
                                                       >::type* = 0)
{  x.swap(y);  }

}  //namespace pdalboost_move_adl_swap{

#else

namespace pdalboost_move_adl_swap{

template<class T>
void swap_proxy(T& x, T& y)
{
   using std::swap;
   swap(x, y);
}

}  //namespace pdalboost_move_adl_swap{

#endif   //#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

namespace pdalboost_move_adl_swap{

template<class T, std::size_t N>
void swap_proxy(T (& x)[N], T (& y)[N])
{
   for (std::size_t i = 0; i < N; ++i){
      ::pdalboost_move_adl_swap::swap_proxy(x[i], y[i]);
   }
}

}  //namespace pdalboost_move_adl_swap {

#endif   //!defined(BOOST_MOVE_DOXYGEN_INVOKED)

namespace pdalboost{

//! Exchanges the values of a and b, using Argument Dependent Lookup (ADL) to select a
//! specialized swap function if available. If no specialized swap function is available,
//! std::swap is used.
//!
//! <b>Exception</b>: If T uses Boost.Move's move emulation and the compiler has
//! no rvalue references then:
//!
//!   -  If T has a <code>T::swap(T&)</code> member, that member is called.
//!   -  Otherwise a move-based swap is called, equivalent to: 
//!      <code>T t(::pdalboost::move(x)); x = ::pdalboost::move(y); y = ::pdalboost::move(t);</code>.
template<class T>
void adl_move_swap(T& x, T& y)
{
   ::pdalboost_move_adl_swap::swap_proxy(x, y);
}

}  //namespace pdalboost{

#endif   //#ifndef BOOST_MOVE_ADL_MOVE_SWAP_HPP
