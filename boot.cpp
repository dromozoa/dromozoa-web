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
#include <cstring>
#include <iostream>
#include "common.hpp"
#include "lua.hpp"
#include "module.hpp"

namespace dromozoa {
  namespace {
    void impl_boot(lua_State* L) {
      static constexpr char code[] =
      #include "boot.lua"
      ;

      luaL_openlibs(L);
      preload(L, "dromozoa.web", function<luaopen_dromozoa_web>());
      preload(L, "dromozoa.web.async", function<luaopen_dromozoa_web_async>());

      if (luaL_loadbuffer(L, code, std::strlen(code), "@boot.lua") != LUA_OK) {
        lua_error(L);
      }
      lua_call(L, 0, 1);
    }

    void impl_each(lua_State* L) {
      lua_pushvalue(L, 1);
      lua_call(L, 0, 0);
    }

    void each(void* state) {
      auto* L = static_cast<lua_State*>(state);
      push(L, function<impl_each>());
      lua_pushvalue(L, -2);
      if (auto e = protected_call(L, 1, 0)) {
        std::cerr << *e << "\n";
        emscripten_cancel_main_loop();
        lua_close(L);
        L = nullptr;
      }
    }

    int boot() {
      auto* L = luaL_newstate();
      if (!L) {
        std::cerr << "cannot create a new state\n";
        return 1;
      }

      push(L, function<impl_boot>());
      if (auto e = protected_call(L, 0, 1)) {
        std::cerr << *e << "\n";
        lua_close(L);
        L = nullptr;
        return 1;
      }

      emscripten_set_main_loop_arg(each, L, 0, false);
      return 0;
    }
  }
}

int main() {
  using namespace dromozoa;
  return boot();
}
