/* Copyright (C) 2015-2016 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef COMMON_ENUM_FLAGS_H
#define COMMON_ENUM_FLAGS_H

/* Type-safe wrapper for enum flags.  enum flags are enums where the
   values are bits that are meant to be ORed together.

   This allows writing code like the below, while with raw enums this
   would fail to compile without casts to enum type at the assignments
   to 'f':

    enum some_flag
    {
       flag_val1 = 1 << 1,
       flag_val2 = 1 << 2,
       flag_val3 = 1 << 3,
       flag_val4 = 1 << 4,
    };
    DEF_ENUM_FLAGS_TYPE(enum some_flag, some_flags)

    some_flags f = flag_val1 | flag_val2;
    f |= flag_val3;

   It's also possible to assign literal zero to an enum flags variable
   (meaning, no flags), dispensing adding an awkward explicit "no
   value" value to the enumeration.  For example:

    some_flags f = 0;
    f |= flag_val3 | flag_val4;

   Note that literal integers other than zero fail to compile:

    some_flags f = 1; // error
*/

/* Comprehensive unit tests are found in gdb/enum-flags-selftests.c.  */

#ifdef __cplusplus

#include <type_traits>

/* Traits type used to prevent the global operator overloads from
   instantiating for non-flag enums.  */
template<typename T> struct enum_flags_type {};

/* Use this to mark an enum as flags enum, enabling the global
   operator overloads for ENUM_TYPE.  This must be called in the
   global namespace.  */
#define ENABLE_ENUM_FLAGS_OPERATORS(enum_type)		\
  template<>						\
  struct enum_flags_type<enum_type>			\
  {							\
    typedef enum_flags<enum_type> type;			\
  }

/* Use this to mark an enum as flags enum.  It defines FLAGS_TYPE as
   enum_flags wrapper class for ENUM, and enables the global operator
   overloads for ENUM.  This must be called in the global
   namespace.  */
#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type)	\
  typedef enum_flags<enum_type> flags_type;		\
  ENABLE_ENUM_FLAGS_OPERATORS(enum_type);

template <typename E>
class enum_flags
{
public:
  typedef E enum_type;
  typedef typename std::underlying_type<enum_type>::type underlying_type;

private:
  /* Private type used to support initializing flag types with zero:

       foo_flags f = 0;

     but not other integers:

       foo_flags f = 1;

     The way this works is that we define an implicit constructor that
     takes a pointer to this private type.  Since nothing can
     instantiate an object of this type, the only possible pointer to
     pass to the constructor is the NULL pointer, or, zero.  */
  struct zero_type;

public:
  /* Allow default construction, just like raw enums.  */
  enum_flags () = default;

  /* The default move/copy/assignment do the right thing.  */

  /* If you get an error saying these two overloads are ambiguous,
     then you tried to mix values of different enum types.  */
  constexpr enum_flags (enum_type e)
    : m_enum_value (e)
  {}
  constexpr enum_flags (enum_flags::zero_type *zero)
    : m_enum_value ((enum_type) 0)
  {}

  enum_flags &operator&= (enum_flags e)
  {
    m_enum_value = (enum_type) (m_enum_value & e.m_enum_value);
    return *this;
  }
  enum_flags &operator|= (enum_flags e)
  {
    m_enum_value = (enum_type) (m_enum_value | e.m_enum_value);
    return *this;
  }
  enum_flags &operator^= (enum_flags e)
  {
    m_enum_value = (enum_type) (m_enum_value ^ e.m_enum_value);
    return *this;
  }

  /* Like raw enums, allow conversion to the underlying type.  */
  constexpr operator underlying_type () const
  { return m_enum_value; }

  /* Get the underlying value as a raw enum.  */
  constexpr enum_type raw () const
  { return m_enum_value; }

  /* Binary operations involving some unrelated type (which would be a
     bug) are implemented as non-members, and deleted.  */

  /* Unary operators.  */

  constexpr enum_flags operator~ () const
  { return (enum_type) ~m_enum_value; }

private:
  /* Stored as enum_type because GDB knows to print the bit flags
     neatly if the enum values look like bit flags.  */
  enum_type m_enum_value;
};

/* Global operator overloads.  */

/* Generate binary operators.  */

#define ENUM_FLAGS_GEN_BINOP(OPERATOR_OP, OP)				\
									\
  /* Raw enum on both LHS/RHS.  Returns raw enum type.  */		\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type::enum_type	\
  OPERATOR_OP (enum_type e1, enum_type e2)				\
  {									\
    using underlying = typename std::underlying_type<enum_type>::type;	\
    return (enum_type) (underlying (e1) OP underlying (e2));		\
  }									\
									\
  /* enum_flags on the LHS.  */						\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (enum_flags<enum_type> e1, enum_type e2)			\
  { return e1.raw () OP e2; }						\
									\
  /* enum_flags on the RHS.  */						\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (enum_type e1, enum_flags<enum_type> e2)			\
  { return e1 OP e2.raw (); }						\
									\
  /* enum_flags on both LHS/RHS.  */					\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (enum_flags<enum_type> e1, enum_flags<enum_type> e2)	\
  { return e1.raw () OP e2.raw (); }					\
									\
  /* Delete cases involving unrelated types.  */			\
									\
  template <typename enum_type, typename unrelated_type>		\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (enum_type e1, unrelated_type e2) = delete;		\
									\
  template <typename enum_type, typename unrelated_type>		\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (unrelated_type e1, enum_type e2) = delete;		\
									\
  template <typename enum_type, typename unrelated_type>		\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (enum_flags<enum_type> e1, unrelated_type e2) = delete;	\
									\
  template <typename enum_type, typename unrelated_type>		\
  constexpr typename enum_flags_type<enum_type>::type			\
  OPERATOR_OP (unrelated_type e1, enum_flags<enum_type> e2) = delete;

/* Generate non-member compound assignment operators.  Only the raw
   enum versions are defined here.  The enum_flags versions are
   defined as member functions, simply because it's less code that
   way.

   Note we delete operators that would allow e.g.,

     "enum_type | 1" or "enum_type1 | enum_type2"

   because that would allow a mistake like :
     enum flags1 { F1_FLAGS1 = 1 };
     enum flags2 { F2_FLAGS2 = 2 };
     enum flags1 val;
     switch (val) {
       case F1_FLAGS1 | F2_FLAGS2:
     ...

   If you really need to 'or' different flags, cast to integer first.
*/
#define ENUM_FLAGS_GEN_COMPOUND_ASSIGN(OPERATOR_OP, OP)			\
  /* lval reference version.  */					\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type::enum_type &	\
  OPERATOR_OP (enum_type &e1, const enum_type &e2)			\
  { return e1 = e1 OP e2; }						\
									\
  /* rval reference version.  */					\
  template <typename enum_type>						\
  constexpr typename enum_flags_type<enum_type>::type::enum_type &	\
  OPERATOR_OP (enum_type &&e1, const enum_type &e2)			\
  { return e1 = e1 OP e2; }						\
									\
  /* Delete assignment from unrelated types.  */			\
									\
  template <typename enum_type, typename other_enum_type>		\
  constexpr typename enum_flags_type<enum_type>::type::enum_type &	\
  OPERATOR_OP (enum_type &e1, const other_enum_type &e2) = delete;	\
									\
  template <typename enum_type, typename other_enum_type>		\
  constexpr typename enum_flags_type<enum_type>::type::enum_type &	\
  OPERATOR_OP (enum_type &&e1, const other_enum_type &e2) = delete;

ENUM_FLAGS_GEN_BINOP(operator|, |)
ENUM_FLAGS_GEN_BINOP(operator&, &)
ENUM_FLAGS_GEN_BINOP(operator^, ^)

ENUM_FLAGS_GEN_COMPOUND_ASSIGN(operator|=, |)
ENUM_FLAGS_GEN_COMPOUND_ASSIGN(operator&=, &)
ENUM_FLAGS_GEN_COMPOUND_ASSIGN(operator^=, ^)

/* Unary operators for the raw flags enum.  */

template <typename enum_type>
constexpr typename enum_flags_type<enum_type>::type::enum_type
operator~ (enum_type e)
{
  using underlying = typename std::underlying_type<enum_type>::type;
  return (enum_type) ~underlying (e);
}

/* Delete operator<< and operator>>.  */

template <typename enum_type, typename any_type>
constexpr typename enum_flags_type<enum_type>::type
operator<< (const enum_type &, const any_type &) = delete;

template <typename enum_type, typename any_type>
constexpr typename enum_flags_type<enum_type>::type
operator<< (const enum_flags<enum_type> &, const any_type &) = delete;

template <typename enum_type, typename any_type>
constexpr typename enum_flags_type<enum_type>::type
operator>> (const enum_type &, const any_type &) = delete;

template <typename enum_type, typename any_type>
constexpr typename enum_flags_type<enum_type>::type
operator>> (const enum_flags<enum_type> &, const any_type &) = delete;

#else /* __cplusplus */

/* In C, the flags type is just a typedef for the enum type.  */

#define DEF_ENUM_FLAGS_TYPE(enum_type, flags_type) \
  typedef enum_type flags_type

#endif /* __cplusplus */

#endif /* COMMON_ENUM_FLAGS_H */
