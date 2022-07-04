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

#ifndef DROMOZOA_WEB_UDATA_HPP
#define DROMOZOA_WEB_UDATA_HPP

#include <new>
#include <type_traits>
#include <utility>
#include "lua.hpp"

namespace dromozoa {
  template <class T, class... T_args>
  inline T* new_udata(lua_State* L, T_args&&... args) {
    auto* self = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
    new(self) T(std::forward<T_args>(args)...);
    luaL_setmetatable(L, T::NAME);
    return self;
  }

  template <class T>
  inline T* check_udata(lua_State* L, int index) {
    return static_cast<T*>(luaL_checkudata(L, index, T::NAME));
  }

  template <class T>
  inline T* test_udata(lua_State* L, int index) noexcept {
    try {
      return static_cast<T*>(luaL_testudata(L, index, T::NAME));
    } catch (...) {}
    return nullptr;
  }

  template <class T>
  inline int gc_udata(lua_State* L) noexcept {
    static_assert(std::is_nothrow_destructible_v<T>);
    if (auto* self = test_udata<T>(L, 1)) {
      self->~T();
    }
    return 0;
  }

  template <class T>
  inline int close_udata(lua_State* L) noexcept {
    static_assert(std::is_nothrow_invocable_v<decltype(&T::close), T*>);
    if (auto* self = test_udata<T>(L, 1)) {
      self->close();
    }
    return 0;
  }
}

#endif
