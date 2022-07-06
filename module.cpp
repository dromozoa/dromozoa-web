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

#include "array.hpp"
#include "browser.hpp"
#include "error.hpp"
#include "error_queue.hpp"
#include "lua.hpp"
#include "object.hpp"
#include "stack_guard.hpp"
#include "thread.hpp"
#include "utility.hpp"

extern "C" {
  using namespace dromozoa;

  void luaopen_dromozoa_web(lua_State* L) {
    lua_newtable(L);
    initialize_array(L);
    initialize_browser(L);
    initialize_error(L);
    initialize_error_queue(L);
    initialize_object(L);
    initialize_thread(L);
    initialize_utility(L);
  }

  void luaopen_dromozoa_web_async(lua_State* L) {
    static const char code[] =
    #include "async.lua"
    ;

    stack_guard guard(L);
    if (luaL_loadbuffer(L, code, std::strlen(code), "@dromozoa/web/async.lua") != LUA_OK) {
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
}
