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

#include <cstddef>
#include "common.hpp"
#include "js_asm.hpp"
#include "js_push.hpp"
#include "lua.hpp"
#include "object.hpp"
#include "udata.hpp"
#include "utility.hpp"

namespace dromozoa {
  namespace {
    void impl_import(lua_State* L) {
      auto top = lua_gettop(L);

      lua_pushvalue(L, lua_upvalueindex(1));
      for (int i = 1; i <= top; ++i) {
        lua_pushvalue(L, 1);
        lua_gettable(L, lua_upvalueindex(1));
      }
    }

    void impl_new(lua_State* L) {
      auto self = check_udata<object>(L, 1);
      auto top = lua_gettop(L);

      DROMOZOA_JS_ASM(D.args = []);
      for (int i = 2; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM(D.args.push(D.stack.pop()));
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.args = undefined;
        D.push($0, new D.objs[$1](...args));
      }, L, self->get());
    }

    void impl_ref(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, D.stack.pop()), L);
    }

    void impl_typeof(lua_State* L) {
      if (js_push_not_throw_unexpcted(L, 1)) {
        DROMOZOA_JS_ASM(D.push($0, typeof D.stack.pop()), L);
      } else {
        lua_pushnil(L);
      }
    }

    void impl_instanceof(lua_State* L) {
      auto* that = check_udata<object>(L, 2);
      if (js_push_not_throw_unexpcted(L, 1)) {
        DROMOZOA_JS_ASM(D.push($0, D.stack.pop() instanceof D.objs[$1]), L, that->get());
      } else {
        push(L, false);
      }
    }

    void impl_is_truthy(lua_State* L) {
      if (js_push_not_throw_unexpcted(L, 1)) {
        DROMOZOA_JS_ASM(D.push($0, !!D.stack.pop()), L);
      } else {
        push(L, true);
      }
    }

    void impl_is_falsy(lua_State* L) {
      if (js_push_not_throw_unexpcted(L, 1)) {
        DROMOZOA_JS_ASM(D.push($0, !D.stack.pop()), L);
      } else {
        push(L, false);
      }
    }

    void impl_slice(lua_State* L) {
      std::size_t size = 0;
      const auto* data = luaL_checklstring(L, 1, &size);
      DROMOZOA_JS_ASM(D.push($0, HEAPU8.slice($1, $2)), L, data, data + size);
    }
  }

  void initialize_utility(lua_State* L) {
    lua_pushvalue(L, -1);
    set_field(L, -2, "import", function<impl_import, 1>());

    set_field(L, -1, "new", function<impl_new>());
    set_field(L, -1, "ref", function<impl_ref>());
    set_field(L, -1, "typeof", function<impl_typeof>());
    set_field(L, -1, "instanceof", function<impl_instanceof>());
    set_field(L, -1, "is_truthy", function<impl_is_truthy>());
    set_field(L, -1, "is_falsy", function<impl_is_falsy>());
    set_field(L, -1, "slice", function<impl_slice>());
  }
}
