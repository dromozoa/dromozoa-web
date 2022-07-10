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
#include "js_asm.hpp"
#include "js_push.hpp"
#include "lua.hpp"
#include "object.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    void impl_eq(lua_State* L) {
      auto* self = test_udata<object>(L, 1);
      auto* that = test_udata<object>(L, 2);
      if (self && that) {
        DROMOZOA_JS_ASM(D.push($0, D.objs[$1] === D.objs[$2]), L, self->get(), that->get());
      } else {
        push(L, false);
      }
    }

    void impl_index(lua_State* L) {
      auto* self = check_udata<object>(L, 1);
      switch (lua_type(L, 2)) {
        case LUA_TNUMBER:
          DROMOZOA_JS_ASM(D.push($0, D.objs[$1][$2]), L, self->get(), lua_tonumber(L, 2));
          break;
        case LUA_TSTRING:
          DROMOZOA_JS_ASM(D.push($0, D.objs[$1][UTF8ToString($2)]), L, self->get(), lua_tostring(L, 2));
          break;
        case LUA_TUSERDATA:
          if (auto* that = test_udata<object>(L, 2)) {
            if (that->is_symbol()) {
              DROMOZOA_JS_ASM(D.push($0, D.objs[$1][D.objs[$2]]), L, self->get(), that->get());
              break;
            }
          }
          throw DROMOZOA_LOGIC_ERROR("!!!");
          break;
        default:
          luaL_typeerror(L, 2, "number or string");
      }
    }

    void impl_newindex(lua_State* L) {
      auto* self = check_udata<object>(L, 1);
      switch (lua_type(L, 2)) {
        case LUA_TNUMBER:
          js_push(L, 3);
          DROMOZOA_JS_ASM(D.objs[$0][$1] = D.stack.pop(), self->get(), lua_tonumber(L, 2));
          break;
        case LUA_TSTRING:
          js_push(L, 3);
          DROMOZOA_JS_ASM(D.objs[$0][UTF8ToString($1)] = D.stack.pop(), self->get(), lua_tostring(L, 2));
          break;
        default:
          luaL_typeerror(L, 2, "number or string");
      }
    }

    void impl_call(lua_State* L) {
      auto* self = check_udata<object>(L, 1);
      auto top = lua_gettop(L);

      DROMOZOA_JS_ASM(D.args = []);
      for (int i = 2; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM(D.args.push(D.stack.pop()));
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.args = undefined;
        const result = D.objs[$1].call(...args);
        if (result === undefined) {
          D.push($0, args[0]);
        } else {
          D.push($0, result);
        }
      }, L, self->get());
    }

    void impl_tostring(lua_State* L) {
      auto* self = check_udata<object>(L, 1);
      DROMOZOA_JS_ASM(D.push($0, D.objs[$1].toString()), L, self->get());
    }
  }

  void object::close() noexcept {
    if (ref_) {
      try {
        DROMOZOA_JS_ASM(D.unref_object($0), ref_);
      } catch (...) {}
      ref_ = 0;
    }
  }

  void initialize_object(lua_State* L) {
    luaL_newmetatable(L, object::NAME);
    set_field(L, -1, "__eq", function<impl_eq>());
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
    set_field(L, -1, "__call", function<impl_call>());
    set_field(L, -1, "__tostring", function<impl_tostring>());
    set_field(L, -1, "__close", close_udata<object>);
    set_field(L, -1, "__gc", gc_udata<object>);
    lua_pop(L, 1);

    DROMOZOA_JS_ASM(D.push($0, globalThis), L);
    lua_setfield(L, -2, "global");

    set_field(L, -1, "null", nullptr);
  }
}
