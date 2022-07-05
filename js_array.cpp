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

#include "common.hpp"
#include "js_array.hpp"
#include "lua.hpp"

namespace dromozoa {
  namespace {
    constexpr char NAME[] = "dromozoa.web.js_array";

    void impl_array(lua_State* L) {
      luaL_setmetatable(L, NAME);
    }
  }

  bool is_js_array(lua_State* L, int index) {
    stack_guard guard(L);
    if (luaL_getmetafield(L, index, NAME) != LUA_TNIL) {
      return lua_toboolean(L, -1);
    }
    return false;
  }

  void initialize_js_array(lua_State* L) {
    luaL_newmetatable(L, NAME);
    set_field(L, -1, NAME, true);
    lua_pop(L, 1);

    set_field(L, -1, "array", function<impl_array>());
  }
}
