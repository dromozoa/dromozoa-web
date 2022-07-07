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
#include "utility.hpp"

namespace dromozoa {
  namespace {
    void impl_new(lua_State* L) {
      auto top = lua_gettop(L);

      js_push(L, 1);
      DROMOZOA_JS_ASM({
        D.cstr = D.stack.pop();
        D.args = [];
      });

      for (auto i = 2; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM(D.args.push(D.stack.pop()));
      }

      DROMOZOA_JS_ASM({
        const cstr = D.cstr;
        const args = D.args;
        D.cstr = undefined;
        D.args = undefined;
        D.push($0, new cstr(...args));
      }, L);
    }

    void impl_ref(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, D.stack.pop()), L);
    }

    void impl_typeof(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, typeof D.stack.pop()), L);
    }

    void impl_instanceof(lua_State* L) {
      js_push(L, 1);
      js_push(L, 2);
      DROMOZOA_JS_ASM({
        const cstr = D.stack.pop();
        D.push($0, D.stack.pop() instanceof cstr);
      }, L);
    }

    void impl_is_truthy(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, !!D.stack.pop()), L);
    }

    void impl_is_falsy(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, !D.stack.pop()), L);
    }
  }

  void initialize_utility(lua_State* L) {
    set_field(L, -1, "new", function<impl_new>());
    set_field(L, -1, "ref", function<impl_ref>());
    set_field(L, -1, "typeof", function<impl_typeof>());
    set_field(L, -1, "instanceof", function<impl_instanceof>());
    set_field(L, -1, "is_truthy", function<impl_is_truthy>());
    set_field(L, -1, "is_falsy", function<impl_is_falsy>());
  }
}
