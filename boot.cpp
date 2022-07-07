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
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include "common.hpp"
#include "lua.hpp"
#include "module.hpp"
#include "stack_guard.hpp"
#include "thread.hpp"

namespace dromozoa {
  namespace {
    lua_State* L = nullptr;

    std::optional<std::string> boot() {
      luaL_openlibs(L);
      preload(L, "dromozoa.web", function<luaopen_dromozoa_web>());
      preload(L, "dromozoa.web.async", function<luaopen_dromozoa_web_async>());

      static constexpr char CODE[] =
      #include "boot.lua"
      ;

      if (auto e = load_buffer(L, std::string_view(CODE), "@boot.lua")) {
        return e;
      }
      if (auto e = pcall(L, 0, 1)) {
        return e;
      }
      return std::nullopt;
    }

    void each() {
      if (L) {
        stack_guard guard(L);
        lua_pushvalue(L, -1);
        if (auto e = pcall(L, 0, 1)) {
          guard.release();
          std::cerr << *e << "\n";
          emscripten_cancel_main_loop();
          lua_close(L);
          L = nullptr;
        }
      }
    }
  }
}

int main() {
  using namespace dromozoa;

  L = luaL_newstate();
  if (!L) {
    std::cerr << "cannot luaL_newstate\n";
    return 1;
  }

  if (auto e = boot()) {
    std::cerr << *e << "\n";
    lua_close(L);
    L = nullptr;
    return 1;
  }

  emscripten_set_main_loop(each, 0, false);
  return 0;
}
