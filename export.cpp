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
#include "error.hpp"
#include "error_queue.hpp"
#include "js_asm.hpp"
#include "js_push.hpp"
#include "lua.hpp"
#include "object.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    void impl_load_string(lua_State* L) {
      const auto* code = luaL_checkstring(L, 1);
      if (luaL_loadstring(L, code) != LUA_OK) {
        lua_error(L);
      }
    }
  }
}

extern "C" {
  using namespace dromozoa;

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_load_string(lua_State* L, const char* code) {
    push(L, function<impl_load_string>());
    push(L, code);
    if (auto e = protected_call(L, 1, 1)) {
      push_error_queue(*e);
      return 0;
    }
    return 1;
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_call(lua_State* L, int n) {
    auto status = lua_pcall(L, n, 1, 0);
    if (status == LUA_OK) {
      js_push(L, -1);
      return 1;
    }
    if (auto* e = test_udata<error>(L, -1)) {
      DROMOZOA_JS_ASM(D.stack.push(UTF8ToString($0)), e->what());
      return 2;
    }
    push_error_queue(make_protected_call_error(L, -1, status));
    return 0;
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(lua_State* L) {
    lua_pushnil(L);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_number(lua_State* L, double value) {
    lua_pushnumber(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_boolean(lua_State* L, int value) {
    lua_pushboolean(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_string(lua_State* L, const char* value) {
    lua_pushstring(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_null(lua_State* L) {
    lua_pushlightuserdata(L, nullptr);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_object(lua_State* L, int id) {
    new_udata<object>(L, id);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_ref(lua_State* L, int ref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_ref(lua_State* L, int index) {
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_unref(lua_State* L, int ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
}
