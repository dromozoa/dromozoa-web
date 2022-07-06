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
#include "error.hpp"
#include "lua.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    void impl_throw(lua_State* L) {
      new_udata<error>(L, luaL_checkstring(L, 1));
      lua_error(L);
    }
  }

  void initialize_error(lua_State* L) {
    luaL_newmetatable(L, error::NAME);
    set_field(L, -1, "__gc", gc_udata<error>);
    lua_pop(L, 1);

    set_field(L, -1, "throw", function<impl_throw>());
  }
}
