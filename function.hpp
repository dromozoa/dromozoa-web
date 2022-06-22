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

#ifndef DROMOZOA_WEB_FUNCTION_HPP
#define DROMOZOA_WEB_FUNCTION_HPP

#include <exception>
#include <stdexcept>
#include "lua.hpp"

namespace dromozoa {
  namespace detail {
    template <class T>
    struct function_wrapper_impl {
      static void set_field(lua_State* L, int index, const char* key) {
        index = lua_absindex(L, index);
        lua_pushcfunction(L, T::value);
        lua_setfield(L, index, key);
      }

      static void set_metafield(lua_State* L, int index, const char* key) {
        if (lua_getmetatable(L, index)) {
          lua_pushcfunction(L, T::value);
          lua_setfield(L, -2, key);
          lua_pop(L, 1);
        } else {
          index = lua_absindex(L, index);
          lua_newtable(L);
          lua_pushcfunction(L, T::value);
          lua_setfield(L, -2, key);
          lua_setmetatable(L, index);
        }
      }
    };
  }

  template <class T, T (*)(lua_State*)>
  struct function_wrapper;

  template <int (*T)(lua_State*)>
  struct function_wrapper<int, T> : detail::function_wrapper_impl<function_wrapper<int, T> > {
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
  struct function_wrapper<void, T> : detail::function_wrapper_impl<function_wrapper<void, T> > {
    static int value(lua_State* L) {
      try {
        int top = lua_gettop(L);
        T(L);
        int result = lua_gettop(L) - top;
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
  function_wrapper<int, T> function();

  template <void (*T)(lua_State*)>
  function_wrapper<void, T> function();
}

#endif
