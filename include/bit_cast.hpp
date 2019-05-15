/* C++ 20 bit cast emulation.
(C) 2018 - 2019 Niall Douglas <http://www.nedproductions.biz/> (3 commits)
File Created: May 2019


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef QUICKCPPLIB_BIT_CAST_HPP
#define QUICKCPPLIB_BIT_CAST_HPP

#include "config.hpp"

#ifndef QUICKCPPLIB_USE_STD_BIT_CAST
#if __cplusplus >= 202000 && __has_include(<bit>)
#define QUICKCPPLIB_USE_STD_BIT_CAST 1
#else
#define QUICKCPPLIB_USE_STD_BIT_CAST 0
#endif
#endif

#if QUICKCPPLIB_USE_STD_BIT_CAST
#include <bit>
#endif
#include <type_traits>

QUICKCPPLIB_NAMESPACE_BEGIN

namespace bit_cast
{
  //! Namespace for user specialised traits
  namespace traits
  {
    /*! Specialise to true if you guarantee that a type is move relocating (i.e.
    its move constructor equals copying bits from old to new, old is left in a
    default constructed state, and calling the destructor on a default constructed
    instance is trivial). All trivially copyable types are move relocating by
    definition, and that is the unspecialised implementation.
    */
    template <class T> struct is_move_relocating
    {
      static constexpr bool value = std::is_trivially_copyable<T>::value;
    };
  }  // namespace traits

#if QUICKCPPLIB_USE_STD_BIT_CAST
  using std::bit_cast;
#else

  namespace detail
  {
    /* A partially compliant implementation of C++20's std::bit_cast function contributed
    by Jesse Towner.
    Our bit_cast is only guaranteed to be constexpr when both the input and output
    arguments are either integrals or enums. We still attempt a constexpr union-based
    type pun for non-array input types, which some compilers accept. For array inputs,
    we fall back to non-constexpr memmove.
    */

    template <class T> using is_integral_or_enum = std::integral_constant<bool, std::is_integral<T>::value || std::is_enum<T>::value>;

    template <class To, class From> using is_static_castable = std::integral_constant<bool, is_integral_or_enum<To>::value && is_integral_or_enum<From>::value>;

    template <class To, class From> using is_union_castable = std::integral_constant<bool, !is_static_castable<To, From>::value && !std::is_array<To>::value && !std::is_array<From>::value>;

    template <class To, class From> using is_bit_castable = std::integral_constant<bool, sizeof(To) == sizeof(From) && traits::is_move_relocating<To>::value && traits::is_move_relocating<From>::value>;

    template <class To, class From> union bit_cast_union {
      From source;
      To target;
    };
  }  // namespace detail

  /*! \brief Bit cast emulation chosen if types are move relocating or trivally copyable,
  have identical size, and are statically castable. Constexpr.
  */
  template <class To, class From, //
    typename std::enable_if<detail::is_bit_castable<To, From>::value//
    && detail::is_static_castable<To, From>::value//
    && !detail::is_union_castable<To, From>::value//
    , bool>::type = true> //
    constexpr inline To bit_cast(const From &from) noexcept { return static_cast<To>(from); }

  /*! \brief Bit cast emulation chosen if types are move relocating or trivally copyable,
  have identical size, and are union castable. Constexpr.
  */
  template <class To, class From, //
    typename std::enable_if<detail::is_bit_castable<To, From>::value //
    && !detail::is_static_castable<To, From>::value //
    && detail::is_union_castable<To, From>::value//
    , bool>::type = true> //
    constexpr inline To bit_cast(const From &from) noexcept { return detail::bit_cast_union<To, From>{from}.target; }

  /*! \brief Bit cast emulation chosen if an array of types which are move relocating or
  trivally copyable, and have identical size. NOT constexpr.
  */
  template <class To, class From, //
    typename std::enable_if<detail::is_bit_castable<To, From>::value //
    && !detail::is_static_castable<To, From>::value //
    && !detail::is_union_castable<To, From>::value//
    , bool>::type = true> //
    inline To bit_cast(const From &from) noexcept
  {
    detail::bit_cast_union<To, From> ret;
    memmove(&ret.source, &from, sizeof(ret.source));
    return ret.target;
  }

#endif

}  // namespace bit_cast

QUICKCPPLIB_NAMESPACE_END

#endif
