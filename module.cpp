// Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
//
// This file is part of dromozoa-web.
//
// dromozoa-png is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dromozoa-png is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dromozoa-png.  If not, see <http://www.gnu.org/licenses/>.

#include <exception>
#include "lua.hpp"

namespace dromozoa {
  void initialize_fetch(lua_State*);

  void initialize(lua_State* L) {
    initialize_fetch(L);
  }
}

extern "C" int luaopen_dromozoa(lua_State* L) {
  int top = lua_gettop(L);
  try {
    lua_newtable(L);
    dromozoa::initialize(L);
    return 1;
  } catch (const std::exception& e) {
    lua_settop(L, top);
    return luaL_error(L, "%s", e.what());
  } catch (...) {
    lua_settop(L, top);
    return luaL_error(L, "unknown exception");
  }
}
