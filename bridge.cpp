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

namespace dromozoa {
  namespace {
    /*
      event_target:addEventListener(type, listener, use_capture)

      -- onceサポート？
      [node, type, listener, use_capture]

      __gcでnodeが消されたら、イベントリスナのマップも削除する？
      →これはダメ
      →documentからもdocument_fragmentからもたどれなくなったら？
      node.isConnectedでどうにかならない？
      →document_fragmentについてるときはisConnectedはfalse
      →parentNodeをたどる？
      →getRootNode()
      →手動で消すようにしたほうがいい気がする

      Map.deleteは存在していなくてもエラーにならない。

      callback_type関数をつくる
        とりあえず、EventListener？

      function => listener

      javascriptの例外が出る
        →C++の例外が出る
          →Luaのエラーとして捕捉
            →スタックをまきもどさないと、オブジェクトが解放されない？

      JavaScriptが止まっちゃう

     */

    thread_reference ref;

    class object_t : noncopyable {
    public:
      static constexpr char NAME[] = "dromozoa.web.bridge.object";

      explicit object_t(int id) : id_(id) {}

      ~object_t() {
        close();
      }

      int get_id() const {
        return id_;
      };

      void close() {
        if (id_) {
          EM_ASM({ dromozoa_web_bridge.objects.delete($0); }, id_);
          id_ = 0;
        }
      }

      static object_t* test(lua_State* L, int index) {
        return static_cast<object_t*>(luaL_testudata(L, index, NAME));
      }

      static object_t* check(lua_State* L, int index) {
        return static_cast<object_t*>(luaL_checkudata(L, index, NAME));
      }

    private:
      int id_ = 0;
    };

    void js_push(lua_State* L, int index) {
      switch (lua_type(L, index)) {
        case LUA_TNONE:
        case LUA_TNIL:
          EM_ASM({ dromozoa_web_bridge.stack.push(undefined); });
          break;
        case LUA_TNUMBER:
          EM_ASM({ dromozoa_web_bridge.stack.push($0); }, lua_tonumber(L, index));
          break;
        case LUA_TBOOLEAN:
          EM_ASM({ dromozoa_web_bridge.stack.push(!!$0); }, lua_toboolean(L, index));
          break;
        case LUA_TSTRING:
          EM_ASM({ dromozoa_web_bridge.stack.push(UTF8ToString($0)); }, lua_tostring(L, index));
          break;
        case LUA_TFUNCTION:
          {
            // Luaの参照がはずせない
            lua_pushvalue(L, index);
            int ref = luaL_ref(L, LUA_REGISTRYINDEX);
            EM_ASM({
              const D = dromozoa_web_bridge;
              const r = $0;
              const v = (...args) => {
                const L = D.get_state();
                const n = args.length;
                D.push_function(L, r);
                for (let i = 0; i < n; ++i) {
                  D.push(L, args[i])
                }
                D.call_function(L, n);
              };
              D.stack.push(v);
            }, ref);
          }
          break;
        case LUA_TUSERDATA:
          if (object_t* that = object_t::test(L, index)) {
            EM_ASM({
              const D = dromozoa_web_bridge;
              D.stack.push(D.objects.get($0));
            }, that->get_id());
          } else {
            throw DROMOZOA_LOGIC_ERROR("unsupported type");
          }
          break;
        case LUA_TLIGHTUSERDATA:
          if (!lua_touserdata(L, index)) {
            EM_ASM({ dromozoa_web_bridge.stack.push(null); });
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
          EM_ASM({
            const D = dromozoa_web_bridge;
            D.push_boolean($0, D.objects.get($1) === D.objects.get($2));
          }, L, self->get_id(), that->get_id());
          return;
        }
      }
      lua_pushboolean(L, false);
    }

    void impl_index(lua_State* L) {
      auto* self = object_t::check(L, 1);
      if (lua_isnumber(L, 2)) {
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.push($0, D.objects.get($1)[$2]);
        }, L, self->get_id(), lua_tonumber(L, 2));
      } else {
        const auto* key = luaL_checkstring(L, 2);
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.push($0, D.objects.get($1)[UTF8ToString($2)]);
        }, L, self->get_id(), key);
      }
    }

    void impl_newindex(lua_State* L) {
      auto* self = object_t::check(L, 1);
      js_push(L, 3);
      if (lua_isnumber(L, 2)) {
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.objects.get($0)[$1] = D.stack.pop();
        }, self->get_id(), lua_tonumber(L, 2));
      } else {
        const auto* key = luaL_checkstring(L, 2);
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.objects.get($0)[UTF8ToString($1)] = D.stack.pop();
        }, self->get_id(), key);
      }
    }

    void impl_call(lua_State* L) {
      auto* self = object_t::check(L, 1);
      int top = lua_gettop(L);

      js_push(L, 2);
      EM_ASM({
        const D = dromozoa_web_bridge;
        D.thisArg = D.stack.pop();
        D.args = [];
      });

      for (int i = 3; i <= top; ++i) {
        js_push(L, i);
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.args.push(D.stack.pop());
        });
      }

      EM_ASM({
        const D = dromozoa_web_bridge;
        D.push($0, D.objects.get($1).apply(D.thisArg, D.args));
        D.thisArg = undefined;
        D.args = undefined;
      }, L, self->get_id());
    }

    void impl_close(lua_State* L) {
      object_t::check(L, 1)->close();
    }

    void impl_gc(lua_State* L) {
      object_t::check(L, 1)->~object_t();
    }

    void impl_new(lua_State* L) {
      int top = lua_gettop(L);

      EM_ASM({
        dromozoa_web_bridge.args = [];
      });

      for (int i = 1; i <= top; ++i) {
        js_push(L, i);
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.args.push(D.stack.pop());
        });
      }

      EM_ASM({
        const D = dromozoa_web_bridge;
        D.push($0, D.new.apply(undefined, D.args));
        D.args = undefined;
      }, L);
    }
  }

  void initialize_bridge(lua_State* L) {
    ref = thread_reference(L);

    EM_ASM({
      const D = window.dromozoa_web_bridge = {};
      D.id = 0;
      D.objects = new Map();
      D.stack = [];
      D.generate_id = () => { return ++D.id; };
      D.evaluate_lua = cwrap("dromozoa_web_evaluate_lua", null, ["string"]);
      D.get_state = cwrap("dromozoa_web_get_state", "pointer", []);
      D.push_function = cwrap("dromozoa_web_push_function", null, ["number"]);
      D.call_function = cwrap("dromozoa_web_call_function", null, ["pointer", "number"]);
      D.push_nil = cwrap("dromozoa_web_push_nil", null, ["pointer"]);
      D.push_null = cwrap("dromozoa_web_push_null", null, ["pointer"]);
      D.push_integer = cwrap("dromozoa_web_push_integer", null, ["pointer", "number"]);
      D.push_number = cwrap("dromozoa_web_push_number", null, ["pointer", "number"]);
      D.push_boolean = cwrap("dromozoa_web_push_boolean", null, ["pointer", "number"]);
      D.push_string = cwrap("dromozoa_web_push_string", null, ["pointer", "string"]);
      D.push_object = cwrap("dromozoa_web_push_object", null, ["pointer", "number"]);

      D.push = (L, v) => {
        switch (typeof v) {
          case "undefined":
            D.push_nil(L, v);
            break;
          case "number":
            if (Number.isInteger(v)) {
              D.push_integer(L, v);
            } else {
              D.push_number(L, v);
            }
            break;
          case "boolean":
            D.push_boolean(L, !!v);
            break;
          case "string":
            D.push_string(L, v);
            break;
          default:
            if (v === null) {
              D.push_null(L);
            } else {
              const id = D.generate_id();
              D.objects.set(id, v);
              D.push_object(L, id);
            }
        }
      };

      D.new = (T, ...a) => {
        return new T(a);
      };
    });

    luaL_newmetatable(L, object_t::NAME);
    set_field(L, -1, "__eq", function<impl_eq>());
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
    set_field(L, -1, "__call", function<impl_call>());
    set_field(L, -1, "__close", function<impl_close>());
    set_field(L, -1, "__gc", function<impl_gc>());
    lua_pop(L, 1);

    lua_newtable(L);
    {
      set_field(L, -1, "new", function<impl_new>());

      EM_ASM({
        const D = dromozoa_web_bridge;
        const id = D.generate_id();
        D.objects.set(id, window);
        D.push_object($0, id);
      }, L);
      lua_setfield(L, -2, "window");

      lua_pushlightuserdata(L, nullptr);
      lua_setfield(L, -2, "null");
    }
  }
}

extern "C" {
  void EMSCRIPTEN_KEEPALIVE dromozoa_web_evaluate_lua(const char* code) {
    using namespace dromozoa;
    if (auto* L = ref.get()) {
      std::cout << "top: " << lua_gettop(L) << "\n";

      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        std::cerr << "cannot luaL_loadbuffer: " << lua_tostring(L, -1) << "\n";
        return;
      }

      std::cout << "top: " << lua_gettop(L) << "\n";

      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }

      std::cout << "top: " << lua_gettop(L) << "\n";
    }
  }

  void* EMSCRIPTEN_KEEPALIVE dromozoa_web_get_state() {
    return dromozoa::ref.get();
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_function(void* state, int ref) {
    lua_geti(static_cast<lua_State*>(state), LUA_REGISTRYINDEX, ref);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_call_function(void* state, int nargs) {
    if (lua_State* L = static_cast<lua_State*>(state)) {
      if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }
    }
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(void* state) {
    lua_pushnil(static_cast<lua_State*>(state));
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_null(void* state) {
    lua_pushlightuserdata(static_cast<lua_State*>(state), nullptr);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_integer(void* state, int value) {
    lua_pushinteger(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_number(void* state, double value) {
    lua_pushnumber(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_boolean(void* state, int value) {
    lua_pushboolean(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_string(void* state, const char* value) {
    lua_pushstring(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_object(void* state, int id) {
    using namespace dromozoa;
    new_userdata<object_t>(static_cast<lua_State*>(state), object_t::NAME, id);
  }
}
