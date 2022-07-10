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
#include "error.hpp"
#include "js_asm.hpp"
#include "js_push.hpp"
#include "lua.hpp"
#include "object.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    void js_push_ref(lua_State* L, int index) {
      DROMOZOA_JS_ASM({
        const v = (...args) => {
          const L = D.get_thread();
          if (L) {
            const n = args.length;
            D.push_ref(L, v.ref);
            for (let i = 0; i < n; ++i) {
              D.push(L, args[i]);
            }
            switch (D.call(L, n)) {
              case 1:
                return D.stack.pop();
              case 2:
                throw new Error(D.stack.pop());
            }
          }
        };
        v.ref = D.ref($0, $1);
        D.refs.register(v, v.ref);
        D.stack.push(v);
      }, L, index);
    }

    bool js_push_impl(lua_State* L, int index, bool not_throw_unexpected) {
      switch (lua_type(L, index)) {
        case LUA_TNONE:
        case LUA_TNIL:
          DROMOZOA_JS_ASM(D.stack.push(undefined));
          break;
        case LUA_TNUMBER:
          DROMOZOA_JS_ASM(D.stack.push($0), lua_tonumber(L, index));
          break;
        case LUA_TBOOLEAN:
          DROMOZOA_JS_ASM(D.stack.push(!!$0), lua_toboolean(L, index));
          break;
        case LUA_TSTRING:
          DROMOZOA_JS_ASM(D.stack.push(UTF8ToString($0)), lua_tostring(L, index));
          break;
        case LUA_TTABLE:
          {
            index = lua_absindex(L, index);

            double origin = is_array(L, index);
            if (origin) {
              DROMOZOA_JS_ASM(D.stack.push([]));
            } else {
              DROMOZOA_JS_ASM(D.stack.push({}));
            }

            lua_pushnil(L);
            while (lua_next(L, index)) {
              switch (lua_type(L, -2)) {
                case LUA_TNUMBER:
                  DROMOZOA_JS_ASM(D.stack.push($0), lua_tonumber(L, -2) - origin);
                  break;
                case LUA_TSTRING:
                  DROMOZOA_JS_ASM(D.stack.push(UTF8ToString($0)), lua_tostring(L, -2));
                  break;
                case LUA_TUSERDATA:
                  if (auto* that = test_udata<object>(L, -2)) {
                    if (that->is_symbol()) {
                      DROMOZOA_JS_ASM(D.stack.push(D.objs[$0]), that->get());
                      break;
                    }
                  }
                  lua_pop(L, 1);
                  continue;
                default:
                  lua_pop(L, 1);
                  continue;
              }
              if (js_push_impl(L, -1, true)) {
                DROMOZOA_JS_ASM({
                  const v = D.stack.pop();
                  const k = D.stack.pop();
                  D.stack[D.stack.length - 1][k] = v;
                });
              } else {
                DROMOZOA_JS_ASM(D.stack.pop());
              }
              lua_pop(L, 1);
            }
          }
          break;
        case LUA_TFUNCTION:
          js_push_ref(L, index);
          break;
        case LUA_TUSERDATA:
          if (auto* that = test_udata<object>(L, index)) {
            DROMOZOA_JS_ASM(D.stack.push(D.objs[$0]), that->get());
          } else {
            js_push_ref(L, index);
          }
          break;
        case LUA_TLIGHTUSERDATA:
          if (!lua_touserdata(L, index)) {
            DROMOZOA_JS_ASM(D.stack.push(null));
          } else {
            if (not_throw_unexpected) {
              return false;
            } else {
              throw DROMOZOA_LOGIC_ERROR("unexpected lightuserdata");
            }
          }
          break;
        default:
          if (not_throw_unexpected) {
            return false;
          } else {
            throw DROMOZOA_LOGIC_ERROR("unexpected ", luaL_typename(L, index));
          }
      }
      return true;
    }
  }

  void js_push(lua_State* L, int index) {
    js_push_impl(L, index, false);
  }

  bool js_push_not_throw_unexpcted(lua_State* L, int index) {
    return js_push_impl(L, index, true);
  }
}
