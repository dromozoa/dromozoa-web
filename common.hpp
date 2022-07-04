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

#include <cstddef>
#include <limits>
#include <new>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include "lua.hpp"
#include "noncopyable.hpp"
#include "stack_guard.hpp"

namespace dromozoa {
  template <class T, T (*)(lua_State*)>
  struct function_wrapper;

  template <int (*T)(lua_State*)>
  struct function_wrapper<int, T> {
    static int value(lua_State* L) {
      try {
        return T(L);
      } catch (const std::runtime_error& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
      } catch (const std::exception& e) {
        return luaL_error(L, "%s", e.what());
      }
    }
  };

  template <void (*T)(lua_State*)>
  struct function_wrapper<void, T> {
    static int value(lua_State* L) {
      try {
        auto top = lua_gettop(L);
        T(L);
        auto result = lua_gettop(L) - top;
        if (result > 0) {
          return result;
        } else {
          if (lua_toboolean(L, 1)) {
            lua_pushvalue(L, 1);
          } else {
            lua_pushboolean(L, true);
          }
          return 1;
        }
      } catch (const std::runtime_error& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
      } catch (const std::exception& e) {
        return luaL_error(L, "%s", e.what());
      }
    }
  };

  template <int (*T)(lua_State*)>
  function_wrapper<int, T> function() {
    return function_wrapper<int, T>();
  }

  template <void (*T)(lua_State*)>
  function_wrapper<void, T> function() {
    return function_wrapper<void, T>();
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && sizeof(T) < sizeof(lua_Integer) + std::is_signed_v<T>), std::nullptr_t> = nullptr>
  inline void push(lua_State* L, T value) {
    lua_pushinteger(L, static_cast<lua_Integer>(value));
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) > sizeof(lua_Integer)), std::nullptr_t> = nullptr>
  inline void push(lua_State* L, T value) {
    static constexpr T min = std::numeric_limits<lua_Integer>::min();
    static constexpr T max = std::numeric_limits<lua_Integer>::max();
    if (min <= value && value <= max) {
      lua_pushinteger(L, static_cast<lua_Integer>(value));
    } else {
      lua_pushnumber(L, static_cast<lua_Number>(value));
    }
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(T) >= sizeof(lua_Integer)), std::nullptr_t> = nullptr>
  inline void push(lua_State* L, T value) {
    static constexpr T max = std::numeric_limits<lua_Integer>::max();
    if (value <= max) {
      lua_pushinteger(L, static_cast<lua_Integer>(value));
    } else {
      lua_pushnumber(L, static_cast<lua_Number>(value));
    }
  }

  template <class T, std::enable_if_t<std::is_floating_point_v<T>, std::nullptr_t> = nullptr>
  inline void push(lua_State* L, T value) {
    lua_pushnumber(L, static_cast<lua_Number>(value));
  }

  inline void push(lua_State* L, const char* value) {
    lua_pushstring(L, value);
  }

  inline void push(lua_State* L, const std::string& value) {
    lua_pushlstring(L, value.data(), value.size());
  }

  inline void push(lua_State* L, bool value) {
    lua_pushboolean(L, value);
  }

  template <class T, T (*T_function)(lua_State*)>
  inline void push(lua_State* L, function_wrapper<T, T_function>) {
    lua_pushcfunction(L, (function_wrapper<T, T_function>::value));
  }

  inline void push(lua_State* L, lua_CFunction value) {
    lua_pushcfunction(L, value);
  }

  inline void push(lua_State* L, std::nullptr_t) {
    lua_pushlightuserdata(L, nullptr);
  }

  template <class T_key, class T_value>
  inline void set_field(lua_State* L, int index, T_key&& key, T_value&& value) {
    index = lua_absindex(L, index);
    push(L, std::forward<T_key>(key));
    push(L, std::forward<T_value>(value));
    lua_settable(L, index);
  }

  template <class T_key>
  inline void set_field(lua_State* L, int index, T_key&& key) {
    index = lua_absindex(L, index);
    push(L, std::forward<T_key>(key));
    lua_pushvalue(L, -2);
    lua_settable(L, index);
    lua_pop(L, 1);
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(lua_Integer) <= sizeof(T)), std::nullptr_t> = nullptr>
  inline std::optional<T> integral_cast(lua_Integer value) {
    return static_cast<T>(value);
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(lua_Integer) > sizeof(T)), std::nullptr_t> = nullptr>
  inline std::optional<T> integral_cast(lua_Integer value) {
    static constexpr lua_Integer min = std::numeric_limits<T>::min();
    static constexpr lua_Integer max = std::numeric_limits<T>::max();
    if (min <= value && value <= max) {
      return static_cast<T>(value);
    } else {
      return std::nullopt;
    }
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(lua_Integer) <= sizeof(T)), std::nullptr_t> = nullptr>
  inline std::optional<T> integral_cast(lua_Integer value) {
    if (0 <= value) {
      return static_cast<T>(value);
    } else {
      return std::nullopt;
    }
  }

  template <class T, std::enable_if_t<(std::is_integral_v<T> && std::is_unsigned_v<T> && sizeof(lua_Integer) > sizeof(T)), std::nullptr_t> = nullptr>
  inline std::optional<T> integral_cast(lua_Integer value) {
    static constexpr lua_Integer max = std::numeric_limits<T>::max();
    if (0 <= value && value < max) {
      return static_cast<T>(value);
    } else {
      return std::nullopt;
    }
  }

  template <class T>
  inline std::optional<T> get_field_integer(lua_State* L, int index, const char* key) {
    stack_guard guard(L);
    if (lua_getfield(L, index, key) != LUA_TNIL) {
      int is_integer = 0;
      auto value = lua_tointegerx(L, -1, &is_integer);
      if (!is_integer) {
        luaL_error(guard.release(), "field '%s' is not an integer", key);
      }
      if (auto result = integral_cast<T>(value)) {
        return result;
      } else {
        luaL_error(guard.release(), "field '%s' out of bounds", key);
      }
    }
    return std::nullopt;
  }

  inline std::optional<std::string> get_field_string(lua_State* L, int index, const char* key) {
    stack_guard guard(L);
    if (lua_getfield(L, index, key) != LUA_TNIL) {
      std::size_t size = 0;
      if (const auto* data = lua_tolstring(L, -1, &size)) {
        return std::string(data, size);
      } else {
        luaL_error(guard.release(), "field '%s' is not a string", key);
      }
    }
    return std::nullopt;
  }
}

#endif
