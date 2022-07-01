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
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"
#include "thread_reference.hpp"

#define JS_ASM(code, ...) \
  if (!emscripten_asm_const_int(CODE_EXPR("try{" #code "}catch(e){try{console.log(e);}catch(_){}return 0;}return 1") _EM_ASM_PREP_ARGS(__VA_ARGS__))) \
    throw DROMOZOA_LOGIC_ERROR("javascript error")

namespace dromozoa {
  namespace {
    // thread_reference
    // lua_Stateがcloseされたら閉じたい
    // refしておいて、自動的に閉じられるようにする
    // __gcでクローズする
    // threadをrefに積んでおく
    // さらに、そのポインタ参照をrefにつんでおく
    // モジュール自体に__gcを設定しよう

    lua_State* thread = nullptr;

    class array_t : noncopyable {
    public:
      static constexpr char NAME[] = "dromozoa.web.array";
    };

    class object_t : noncopyable {
    public:
      static constexpr char NAME[] = "dromozoa.web.object";

      explicit object_t(int ref) : ref_(ref) {}

      ~object_t() {
        close();
      }

      int get() const {
        return ref_;
      };

      void close() {
        if (ref_) {
          JS_ASM({ D.unref_object($0); }, ref_);
          ref_ = 0;
        }
      }

      static object_t* test(lua_State* L, int index) {
        return static_cast<object_t*>(luaL_testudata(L, index, NAME));
      }

      static object_t* check(lua_State* L, int index) {
        return static_cast<object_t*>(luaL_checkudata(L, index, NAME));
      }

    private:
      int ref_;
    };

    void js_push(lua_State* L, int index) {
      switch (lua_type(L, index)) {
        case LUA_TNONE:
        case LUA_TNIL:
          JS_ASM({ D.stack.push(undefined); });
          break;
        case LUA_TNUMBER:
          JS_ASM({ D.stack.push($0); }, lua_tonumber(L, index));
          break;
        case LUA_TBOOLEAN:
          JS_ASM({ D.stack.push(!!$0); }, lua_toboolean(L, index));
          break;
        case LUA_TSTRING:
          JS_ASM({ D.stack.push(UTF8ToString($0)); }, lua_tostring(L, index));
          break;
        case LUA_TTABLE:
          {
            index = lua_absindex(L, index);

            // metatableがarrayだったら、
            bool is_array = false;
            if (lua_getmetatable(L, index)) {
              luaL_getmetatable(L, array_t::NAME);
              if (lua_rawequal(L, -1, -2)) {
                is_array = true;
              }
              lua_pop(L, 2);
            }

            if (is_array) {
              JS_ASM({ D.stack.push([]); });
            } else {
              JS_ASM({ D.stack.push({}); });
            }

            lua_pushnil(L);
            while (lua_next(L, index)) {
              // k: -2
              // v: -1
              switch (lua_type(L, -2)) {
                case LUA_TNUMBER:
                  if (is_array && lua_isinteger(L, -2)) {
                    JS_ASM({ D.stack.push($0); }, lua_tonumber(L, -2) - 1);
                  } else {
                    JS_ASM({ D.stack.push($0); }, lua_tonumber(L, -2));
                  }
                  break;
                case LUA_TSTRING:
                  // 文字列なので、スタック上の変換は行われない
                  JS_ASM({ D.stack.push(UTF8ToString($0)); }, lua_tostring(L, -2));
                  break;
                // 数値キーと文字列だけを扱う
                default:
                  lua_pop(L, 1);
                  continue;
              }

              js_push(L, -1);

              JS_ASM({
                const v = D.stack.pop();
                const k = D.stack.pop();
                D.stack[D.stack.length - 1][k] = v;
              });

              lua_pop(L, 1);
            }
          }
          break;
        case LUA_TFUNCTION:
          {
            // 厳密にはtry catchをしないとよくわからないことになる？
            // ロールバックを書くほうがよいかもしれない
            // luaL_refに成功して、registerに失敗した場合、参照が切れる
            // そういう事態が発生するか？
            JS_ASM({
              const v = (...args) => {
                const L = D.get_state();
                const n = args.length;
                D.push_function(L, v.ref);
                for (let i = 0; i < n; ++i) {
                  D.push(L, args[i])
                }
                D.call_function(L, n);
                return D.stack.pop();
              };
              v.ref = D.ref($0, $1);
              D.refs.register(v, v.ref);
              D.stack.push(v);
            }, L, index);
          }
          break;
        case LUA_TUSERDATA:
          if (object_t* that = object_t::test(L, index)) {
            JS_ASM({ D.stack.push(D.objs[$0]); }, that->get());
          } else {
            throw DROMOZOA_LOGIC_ERROR("unsupported type");
          }
          break;
        case LUA_TLIGHTUSERDATA:
          if (!lua_touserdata(L, index)) {
            JS_ASM({ D.stack.push(null); });
          } else {
            throw DROMOZOA_LOGIC_ERROR("unsupported type");
          }
          break;
        default:
          throw DROMOZOA_LOGIC_ERROR("unsupported type");
      }
    }

    void impl_eq(lua_State* L) {
      if (auto* self = object_t::test(L, 1)) {
        if (auto* that = object_t::test(L, 2)) {
          JS_ASM({ D.push_boolean($0, D.objs[$1] === D.objs[$2]); }, L, self->get(), that->get());
          return;
        }
      }
      lua_pushboolean(L, false);
    }

    void impl_index(lua_State* L) {
      auto* self = object_t::check(L, 1);
      if (lua_isnumber(L, 2)) {
        JS_ASM({
          D.push($0, D.objs[$1][$2]);
        }, L, self->get(), lua_tonumber(L, 2));
      } else {
        const auto* key = luaL_checkstring(L, 2);
        JS_ASM({
          D.push($0, D.objs[$1][UTF8ToString($2)]);
        }, L, self->get(), key);
      }
    }

    void impl_newindex(lua_State* L) {
      auto* self = object_t::check(L, 1);
      js_push(L, 3);
      if (lua_isnumber(L, 2)) {
        JS_ASM({ D.objs[$0][$1] = D.stack.pop(); }, self->get(), lua_tonumber(L, 2));
      } else {
        const auto* key = luaL_checkstring(L, 2);
        JS_ASM({ D.objs[$0][UTF8ToString($1)] = D.stack.pop(); }, self->get(), key);
      }
    }

    void impl_call(lua_State* L) {
      auto* self = object_t::check(L, 1);
      int top = lua_gettop(L);

      js_push(L, 2);
      JS_ASM({
        D.thisArg = D.stack.pop();
        D.args = [];
      });

      for (int i = 3; i <= top; ++i) {
        js_push(L, i);
        JS_ASM({ D.args.push(D.stack.pop()); });
      }

      JS_ASM({
        D.push($0, D.objs[$1].apply(D.thisArg, D.args));
        D.thisArg = undefined;
        D.args = undefined;
      }, L, self->get());
    }

    void impl_close(lua_State* L) {
      object_t::check(L, 1)->close();
    }

    void impl_gc(lua_State* L) {
      object_t::check(L, 1)->~object_t();
    }

    void impl_module_gc(lua_State*) {
      thread = nullptr;
    }

    void impl_new(lua_State* L) {
      int top = lua_gettop(L);

      JS_ASM({
        D.args = [];
      });

      for (int i = 1; i <= top; ++i) {
        js_push(L, i);
        JS_ASM({
          D.args.push(D.stack.pop());
        });
      }

      JS_ASM({
        D.push($0, D.new.apply(undefined, D.args));
        D.args = undefined;
      }, L);
    }

    void impl_ref(lua_State* L) {
      // callableをスタックに積んで、object_tに変換する
      js_push(L, 1);
      JS_ASM({ D.push_object($0, D.ref_object(D.stack.pop())); }, L);
    }

    void impl_get(lua_State* L) {
      auto* self = object_t::check(L, 1);
      push(L, self->get());
    }

    void impl_array(lua_State* L) {
      luaL_setmetatable(L, array_t::NAME);
    }
  }

  void initialize(lua_State* L) {
    // thread = thread_reference(L);

    thread = lua_newthread(L);
    luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_newmetatable(L, object_t::NAME);
    set_field(L, -1, "__eq", function<impl_eq>());
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
    set_field(L, -1, "__call", function<impl_call>());
    set_field(L, -1, "__close", function<impl_close>());
    set_field(L, -1, "__gc", function<impl_gc>());
    lua_pop(L, 1);

    luaL_newmetatable(L, array_t::NAME);
    lua_pop(L, 1);

    lua_newtable(L);
    {
      set_metafield(L, -1, "__gc", function<impl_module_gc>());

      set_field(L, -1, "new", function<impl_new>());
      set_field(L, -1, "ref", function<impl_ref>());
      set_field(L, -1, "get", function<impl_get>());
      set_field(L, -1, "array", function<impl_array>());

      JS_ASM({ D.push_object($0, D.ref_object(window)); }, L);
      lua_setfield(L, -2, "window");

      lua_pushlightuserdata(L, nullptr);
      lua_setfield(L, -2, "null");
    }
  }
}

extern "C" {
  void EMSCRIPTEN_KEEPALIVE dromozoa_web_evaluate_lua(const char* code) {
    using namespace dromozoa;
    if (auto* L = thread) {
      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        std::cerr << "cannot luaL_loadbuffer: " << lua_tostring(L, -1) << "\n";
        return;
      }
      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }
    }
  }

  lua_State* EMSCRIPTEN_KEEPALIVE dromozoa_web_get_state() {
    return dromozoa::thread;
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_function(lua_State* L, int ref) {
    lua_geti(L, LUA_REGISTRYINDEX, ref);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_call_function(lua_State* L, int nargs) {
    using namespace dromozoa;
    if (lua_pcall(L, nargs, 1, 0) != LUA_OK) {
      std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
    }
    // 返り値をJavaScriptに返す
    js_push(L, -1);
    // 返り値を削除しないと参照がはずれない
    lua_pop(L, 1);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(lua_State* L) {
    lua_pushnil(L);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_null(lua_State* L) {
    lua_pushlightuserdata(L, nullptr);
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

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_object(lua_State* L, int id) {
    using namespace dromozoa;
    new_userdata<object_t>(L, object_t::NAME, id);
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_ref(lua_State* L, int index) {
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_unref(lua_State* L, int ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
}
