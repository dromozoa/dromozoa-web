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
        D.args = [];
        D.args.constructor = D.stack.pop();
      });

      for (auto i = 2; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM(D.args.push(D.stack.pop()));
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.push($0, new args.constructor(...args));
      }, L);
    }

    void impl_ref(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM(D.push($0, D.stack.pop()), L);
    }
  }

  void initialize_utility(lua_State* L) {
    set_field(L, -1, "new", function<impl_new>());
    set_field(L, -1, "ref", function<impl_ref>());
  }
}
