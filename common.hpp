// Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
//
// This file is part of dromozoa-web.
//
// dromozoa-web is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dromozoa-web is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dromozoa-web.  If not, see <http://www.gnu.org/licenses/>.

#ifndef DROMOZOA_WEB_COMMON_HPP
#define DROMOZOA_WEB_COMMON_HPP

#include <limits>
#include <type_traits>
#include "lua.hpp"

namespace dromozoa {
  template <class T, std::enable_if_t<(std::is_integral_v<T> && sizeof(T) < sizeof(lua_Integer) + std::is_signed_v<T>), std::nullptr_t> = nullptr>
  inline void push_integer(lua_State* L, T value) {
    lua_pushinteger(L, value);
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) > sizeof(lua_Integer)), std::nullptr_t> = nullptr>
  inline void push_integer(lua_State* L, T value) {
    static constexpr T min = std::numeric_limits<lua_Integer>::min();
    static constexpr T max = std::numeric_limits<lua_Integer>::max();
    std::cout << min << "," << max << "," << value << "\n";
    if (min <= value && value <= max) {
      lua_pushinteger(L, static_cast<lua_Integer>(value));
    } else {
      lua_pushnumber(L, static_cast<lua_Number>(value));
    }
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) >= sizeof(lua_Integer)), std::nullptr_t> = nullptr>
  inline void push_integer(lua_State* L, T value) {
    static constexpr T max = std::numeric_limits<lua_Integer>::max();
    if (value <= max) {
      lua_pushinteger(L, static_cast<lua_Integer>(value));
    } else {
      lua_pushnumber(L, static_cast<lua_Number>(value));
    }
  }
}

#endif
