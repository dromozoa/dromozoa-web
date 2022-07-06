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

#include <cstring>
#include <emscripten.h>
#include <exception>
#include <iostream>
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"
#include "module.hpp"
#include "stack_guard.hpp"

namespace dromozoa {
  namespace {
    void boot(lua_State* L) {
      luaL_openlibs(L);
      preload(L, "dromozoa.web", function<luaopen_dromozoa_web>());
      preload(L, "dromozoa.web.async", function<luaopen_dromozoa_web_async>());

      static const char code[] =
      #include "boot.lua"
      ;

      stack_guard guard(L);
      if (luaL_loadbuffer(L, code, std::strlen(code), "@boot.lua") != LUA_OK) {
        if (const auto* e = luaL_tolstring(L, -1, nullptr)) {
          throw DROMOZOA_LOGIC_ERROR(e);
        }
        throw DROMOZOA_LOGIC_ERROR("unknown error");
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        if (const auto* e = luaL_tolstring(L, -1, nullptr)) {
          throw DROMOZOA_LOGIC_ERROR(e);
        }
        throw DROMOZOA_LOGIC_ERROR("unknown error");
      }
      guard.release();
    }

    void each(void* state) {
      auto* L = static_cast<lua_State*>(state);
      try {
        stack_guard guard(L);
        lua_pushvalue(L, -1);
        if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
          return;
        }
        if (const auto* e = luaL_tolstring(L, -1, nullptr)) {
          throw DROMOZOA_LOGIC_ERROR(e);
        }
        throw DROMOZOA_LOGIC_ERROR("unknown error");
      } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
      } catch (...) {
        std::cerr << "unknown error\n";
      }
      emscripten_cancel_main_loop();
      lua_close(L);
    }
  }
}

int main() {
  using namespace dromozoa;

  if (auto* L = luaL_newstate()) {
    try {
      boot(L);
      emscripten_set_main_loop_arg(each, L, 0, false);
      return 0;
    } catch (const std::exception& e) {
      std::cerr << e.what() << "\n";
    } catch (...) {
      std::cerr << "unknown error\n";
    }
    emscripten_cancel_main_loop();
    lua_close(L);
  } else {
    std::cerr << "cannot luaL_newstate\n";
  }
  return 1;
}
