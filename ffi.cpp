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
#include <deque>
#include <exception>
#include "common.hpp"
#include "error.hpp"
#include "js_array.hpp"
#include "js_asm.hpp"
#include "js_error.hpp"
#include "js_object.hpp"
#include "js_push.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"
#include "stack_guard.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    std::deque<std::exception_ptr> error_queue;

    void push_error() {
      error_queue.emplace_back(std::current_exception());
    }

    void impl_get_error(lua_State* L) {
      try {
        if (!error_queue.empty()) {
          auto e = error_queue.front();
          error_queue.pop_front();
          std::rethrow_exception(e);
        }
        lua_pushnil(L);
      } catch (const std::exception& e) {
        lua_pushstring(L, e.what());
      } catch (...) {
        lua_pushstring(L, "unknown error");
      }
    }

    void impl_new(lua_State* L) {
      auto top = lua_gettop(L);

      DROMOZOA_JS_ASM({ D.args = []; });

      for (auto i = 1; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM({ D.args.push(D.stack.pop()); });
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.args = undefined;
        D.push($0, D.new.apply(undefined, args));
      }, L);
    }

    void impl_ref(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM({ D.push_object($0, D.ref_object(D.stack.pop())); }, L);
    }
  }

  void initialize_ffi(lua_State* L) {
    set_field(L, -1, "get_error", function<impl_get_error>());
    set_field(L, -1, "new", function<impl_new>());
    set_field(L, -1, "ref", function<impl_ref>());
  }
}

extern "C" {
  using namespace dromozoa;

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_evaluate(lua_State* L, const char* code) {
    try {
      stack_guard guard(L);
      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot luaL_loadbuffer: ", lua_tostring(L, -1));
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        if (auto* that = test_udata<js_error>(L, -1)) {
          DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, that->what());
          return 2;
        } else {
          throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
        }
      } else {
        js_push(L, -1);
        return 1;
      }
    } catch (...) {
      push_error();
    }
    return 0;
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_call(lua_State* L, int n) {
    try {
      stack_guard guard(L);
      if (lua_pcall(L, n, 1, 0) != LUA_OK) {
        if (auto* that = test_udata<js_error>(L, -1)) {
          DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, that->what());
          return 2;
        } else {
          throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
        }
      } else {
        js_push(L, -1);
        return 1;
      }
    } catch (...) {
      push_error();
    }
    return 0;
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(lua_State* L) {
    lua_pushnil(L);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_integer(lua_State* L, int value) {
    lua_pushinteger(L, value);
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
    new_udata<js_object>(L, id);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_ref(lua_State* L, int ref) {
    lua_geti(L, LUA_REGISTRYINDEX, ref);
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_ref_registry(lua_State* L, int index) {
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_unref_registry(lua_State* L, int ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
}
