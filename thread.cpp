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

#include <emscripten.h>
#include "common.hpp"
#include "thread.hpp"

#include <iostream>

namespace dromozoa {
  namespace {
    lua_State* thread = nullptr;

    void impl_gc(lua_State*) {
      thread = nullptr;
    }
  }

  void initialize_thread(lua_State* L) {
    lua_newtable(L);
    thread = lua_newthread(L);
    lua_setfield(L, -2, "thread");
    set_metafield(L, -1, "__gc", function<impl_gc>());
    luaL_ref(L, LUA_REGISTRYINDEX);
  }
}

extern "C" {
  using namespace dromozoa;

  lua_State* EMSCRIPTEN_KEEPALIVE dromozoa_web_get_thread() {
    return thread;
  }
}
