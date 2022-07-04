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
#include <exception>
#include <iostream>
#include <utility>
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"
#include "js_array.hpp"
#include "js_error.hpp"
#include "stack_guard.hpp"

namespace dromozoa {
  void initialize_core(lua_State*);
  void initialize_ffi(lua_State*);

  namespace {
    template <class T_key, class T_value>
    void preload(lua_State* L, T_key&& key, T_value&& value) {
      stack_guard guard(L);
      lua_getglobal(L, "package");
      lua_getfield(L, -1, "preload");
      set_field(L, -1, std::forward<T_key>(key), std::forward<T_value>(value));
    }

    void open(lua_State* L) {
      lua_newtable(L);
      initialize_core(L);
      initialize_ffi(L);
      initialize_js_array(L);
      initialize_js_error(L);
    }

    void boot(lua_State* L) {
      luaL_openlibs(L);
      preload(L, "dromozoa.web", function<open>());

      static const char code[] =
      #include "boot.lua"
      ;

      if (luaL_loadbuffer(L, code, std::strlen(code), "boot.lua") != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot luaL_loadbuffer: ", lua_tostring(L, -1));
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
      }
    }

    void each(void* state) {
      if (auto* L = static_cast<lua_State*>(state)) {
        try {
          stack_guard guard(L);
          lua_pushvalue(L, -1);
          if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
          }
          return;
        } catch (const std::exception& e) {
          std::cerr << e.what() << "\n";
        }
        emscripten_cancel_main_loop();
        lua_close(L);
      }
    }
  }
}

using namespace dromozoa;

int main() {
  if (lua_State* L = luaL_newstate()) {
    try {
      boot(L);
      emscripten_set_main_loop_arg(each, L, 0, false);
      return 0;
    } catch (const std::exception& e) {
      std::cerr << e.what() << "\n";
    }
  } else {
    std::cerr << "cannot luaL_newstate\n";
  }
  return 1;
}
