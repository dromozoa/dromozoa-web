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
#include "lua.hpp"
#include "noncopyable.hpp"
#include "thread_reference.hpp"

namespace dromozoa {
  namespace {
    /*
      JS側にvoid pointerでコンテキストを持たせる？

      LuaからJavaScriptのオブジェクトを操作する

      EM_ASMから、C関数を呼び出して、直接、Luaのスタックに乗せる

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

      listener = function (ev) {
        luaL_ref(key)
        push(ev)
      }

      map[node] = {
        type: [ listener, use_capture ]
      }

      listenerはLuaにバインドしている
     */

    thread_reference ref;

    class object_t : noncopyable {
    public:
      explicit object_t(int id) : id_(id) {}

      ~object_t() {
        close();
      }

      int get_id() const {
        return id_;
      };

      void close() {
        if (id_) {
          EM_ASM({
            const D = dromozoa_web_bridge;
            D.objects.delete($0);
          }, id_);
          id_ = 0;
        }
      }

    private:
      int id_ = 0;
    };

    object_t* test_object(lua_State* L, int index) {
      return static_cast<object_t*>(luaL_testudata(L, index, "dromozoa.web.bridge.object"));
    }

    object_t* check_object(lua_State* L, int index) {
      return static_cast<object_t*>(luaL_checkudata(L, index, "dromozoa.web.bridge.object"));
    }

    template <class T>
    std::unique_ptr<char, decltype(&std::free)> make_unique_cstr(T ptr) {
      return std::unique_ptr<char, decltype(&std::free)>(reinterpret_cast<char*>(ptr), std::free);
    }

    void impl_eq(lua_State* L) {
      auto* self = test_object(L, 1);
      auto* that = test_object(L, 2);
      if (self && that) {
        EM_ASM({
          const D = dromozoa_web_bridge;
          D.push_boolean($0, D.objects.get($1) === D.objects.get($2));
        }, L, self->get_id(), that->get_id());
      } else {
        lua_pushboolean(L, self == that);
      }
    }

    void impl_index(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* key = luaL_checkstring(L, 2);
      // std::cout << "impl_index " << self << "=" << self->get_id() << " " << key << "\n";

      EM_ASM({
        const D = dromozoa_web_bridge;
        const v = D.objects.get($1)[UTF8ToString($2)];
        switch (typeof v) {
          case "undefined":
            D.push_nil($0);
            break;
          case "boolean":
            D.push_boolean($0, !!v);
            break;
          case "number":
            if (Number.isInteger(v)) {
              D.push_integer($0, v);
            } else {
              D.push_number($0, v);
            }
            break;
          case "string":
            D.push_string($0, v);
            break;
          default:
            if (v === null) {
              D.push_nil($0);
            } else {
              const id = D.generate_id();
              D.objects.set(id, v);
              D.push_object($0, id);
            }
        }
      }, L, self->get_id(), key);
    }

    void impl_newindex(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* key = luaL_checkstring(L, 2);
      // std::cout << "impl_newindex " << self << " " << key << "\n";

      switch (lua_type(L, 3)) {
        case LUA_TNONE:
        case LUA_TNIL:
          EM_ASM({
            dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = undefined;
          }, self->get_id(), key);
          break;
        case LUA_TNUMBER:
          if (lua_isinteger(L, 3)) {
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = $2;
            }, self->get_id(), key, lua_tointeger(L, 3));
          } else {
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = $2;
            }, self->get_id(), key, lua_tonumber(L, 3));
          }
          break;
        case LUA_TBOOLEAN:
          if (lua_toboolean(L, 3)) {
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = true;
            }, self->get_id(), key);
          } else {
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = false;
            }, self->get_id(), key);
          }
          break;
        case LUA_TSTRING:
          EM_ASM({
            dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = UTF8ToString($2);
          }, self->get_id(), key, lua_tostring(L, 3));
          break;
        default:
          if (object_t* that = test_object(L, 3)) {
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = dromozoa_web_bridge.objects.get($2);
            }, self->get_id(), key, that->get_id());
          } else {
            std::cerr << "unsupported type\n";
          }
      }
    }

    void impl_call(lua_State* L) {
      auto* self = check_object(L, 1);
      // std::cout << "impl_call " << self << "=" << self->get_id() << "\n";

      int top = lua_gettop(L);
      for (int i = 2; i <= top; ++i) {
        // std::cout << "  [" << i - 1 << "] " << luaL_typename(L, i) << "\n";
      }

      int id = EM_ASM_INT({
        const D = dromozoa_web_bridge;
        const id = D.generate_id();
        D.objects.set(id, []);
        return id;
      });

      for (int i = 2; i <= top; ++i) {
        switch (lua_type(L, i)) {
          case LUA_TNONE: // これは出現しない
          case LUA_TNIL:
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[$1] = undefined;
            }, id, i - 2);
            break;
          case LUA_TNUMBER:
            if (lua_isinteger(L, i)) {
              EM_ASM({
                dromozoa_web_bridge.objects.get($0)[$1] = $2;
              }, id, i - 2, lua_tointeger(L, i));
            } else {
              EM_ASM({
                dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = $2;
              }, id, i - 2, lua_tonumber(L, i));
            }
            break;
          case LUA_TBOOLEAN:
            if (lua_toboolean(L, i)) {
              EM_ASM({
                dromozoa_web_bridge.objects.get($0)[$1] = true;
              }, id, i - 2);
            } else {
              EM_ASM({
                dromozoa_web_bridge.objects.get($0)[$1] = false;
              }, id, i - 2);
            }
            break;
          case LUA_TSTRING:
            EM_ASM({
              dromozoa_web_bridge.objects.get($0)[$1] = UTF8ToString($2);
            }, id, i - 2, lua_tostring(L, i));
            break;
          default:
            if (object_t* that = test_object(L, i)) {
              // std::cout << "args[" << i - 1 << "] " << that->get_id() << "\n";
              EM_ASM({
                dromozoa_web_bridge.objects.get($0)[$1] = dromozoa_web_bridge.objects.get($2);
              }, id, i - 2, that->get_id());
            } else {
              // 関数呼び出し可能であると仮定する
              // いまのところ、refは解放されない。
              // D.objectsは__gcで解放される
              lua_pushvalue(L, i);
              std::cout << luaL_typename(L, -1) << "\n";
              int ref = luaL_ref(L, LUA_REGISTRYINDEX);
              std::cout << "luaL_ref " << ref << "\n";
              EM_ASM({
                const ref = $2;
                dromozoa_web_bridge.objects.get($0)[$1] = function (ev) {
                  const D = dromozoa_web_bridge;
                  const L = D.get_state();
                  D.push_function(L, ref);
                  const id = D.generate_id();
                  D.objects.set(id, ev);
                  D.push_object(L, id);
                  D.call_function(L, 1);
                };
              }, id, i - 2, ref);
            }
        }
      }

      EM_ASM({
        const D = dromozoa_web_bridge;
        const args = D.objects.get($2);
        D.objects.delete($2);
        const arg = args.shift();
        const v = D.objects.get($1).apply(arg, args);
        switch (typeof v) {
          case "undefined":
            D.push_nil($0);
            break;
          case "boolean":
            D.push_boolean($0, !!v);
            break;
          case "number":
            if (Number.isInteger(v)) {
              D.push_integer($0, v);
            } else {
              D.push_number($0, v);
            }
            break;
          case "string":
            D.push_string($0, v);
            break;
          default:
            if (v === null) {
              D.push_nil($0);
            } else {
              const id = D.generate_id();
              D.objects.set(id, v);
              D.push_object($0, id);
            }
        }
      }, L, self->get_id(), id);
    }

    void impl_close(lua_State* L) {
      auto* self = check_object(L, 1);
      // std::cout << "impl_close " << self << "\n";
      self->close();
    }

    void impl_gc(lua_State* L) {
      auto* self = check_object(L, 1);
      // std::cout << "impl_gc " << self << "\n";
      self->~object_t();
    }

    void impl_get_window(lua_State* L) {
      EM_ASM({
        const D = dromozoa_web_bridge;
        const id = D.generate_id();
        D.objects.set(id, window);
        D.push_object($0, id);
      }, L);
    }
  }

  void initialize_bridge(lua_State* L) {
    ref = thread_reference(L);

    EM_ASM({
      const D = window.dromozoa_web_bridge = {};
      D.id = 0;
      D.generate_id = function () { return ++D.id; };
      D.objects = new Map();
      D.get_state = cwrap("dromozoa_web_get_state", "pointer", []);
      D.push_function = cwrap("dromozoa_web_push_function", null, ["number"]);
      D.call_function = cwrap("dromozoa_web_call_function", null, ["pointer", "number"]);
      D.push_nil = cwrap("dromozoa_web_push_nil", null, ["pointer"]);
      D.push_integer = cwrap("dromozoa_web_push_integer", null, ["pointer", "number"]);
      D.push_number = cwrap("dromozoa_web_push_number", null, ["pointer", "number"]);
      D.push_boolean = cwrap("dromozoa_web_push_boolean", null, ["pointer", "number"]);
      D.push_string = cwrap("dromozoa_web_push_string", null, ["pointer", "string"]);
      D.push_object = cwrap("dromozoa_web_push_object", null, ["pointer", "number"]);
    });

    luaL_newmetatable(L, "dromozoa.web.bridge.object");
    set_field(L, -1, "__eq", function<impl_eq>());
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
    set_field(L, -1, "__call", function<impl_call>());
    set_field(L, -1, "__close", function<impl_close>());
    set_field(L, -1, "__gc", function<impl_gc>());
    lua_pop(L, 1);

    lua_newtable(L);
    {
      set_field(L, -1, "get_window", function<impl_get_window>());
    }
  }
}

extern "C" {
  //
  // event_target:add_event_listener(type, listener, use_capture)
  // listener(event)
  // dromozoa_web_tied_objects[i] = ...
  //
  // double
  // int ref;
  //
  // {"ref":0}
  // querySelector
  // querySelectorAll
  //
  // composite object
  // json / text => 簡単, 効率は悪い, 型をあらわしずらい
  // event to json
  // binary object
  // mallocすればいい？
  //
  // jsonに変換するほうが、いちいちブリッジするより速そう
  // domオブジェクトは、だけど、どうしようもない
  // unopaque
  //

  // event object
  // event.currentTarget
  //
  // serialize戦略

  // lua->c++->js
  // js->c++->lua
  //
  // 構造体をプッシュ
  // Luaに直接？

  void EMSCRIPTEN_KEEPALIVE evaluate_lua(const char* code) {
    using namespace dromozoa;
    if (auto* L = ref.get()) {
      // std::cout << code << "\n";
      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        std::cerr << "cannot luaL_loadbuffer: " << lua_tostring(L, -1) << "\n";
        return;
      }
      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }
    }
  }

  void* EMSCRIPTEN_KEEPALIVE dromozoa_web_get_state() {
    return dromozoa::ref.get();
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_function(void* state, int ref) {
    std::cout << "push_function " << state << " " << ref << "\n";
    auto r = lua_geti(static_cast<lua_State*>(state), LUA_REGISTRYINDEX, ref);
    std::cout << "lua_geti " << r << "\n";
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_call_function(void* state, int nargs) {
    std::cout << "call_function " << state << " " << nargs << "\n";
    if (lua_State* L = static_cast<lua_State*>(state)) {
      if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }
    }
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(void* state) {
    // std::cout << "push_nil " << state << "\n";
    lua_pushnil(static_cast<lua_State*>(state));
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_integer(void* state, int value) {
    // std::cout << "push_integer " << state << " " << value << "\n";
    lua_pushinteger(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_number(void* state, double value) {
    // std::cout << "push_number " << state << " " << value << "\n";
    lua_pushnumber(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_boolean(void* state, int value) {
    // std::cout << "push_boolean " << state << " " << value << "\n";
    lua_pushboolean(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_string(void* state, const char* value) {
    // std::cout << "push_string " << state << " " << value << "\n";
    lua_pushstring(static_cast<lua_State*>(state), value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_object(void* state, int id) {
    // std::cout << "push_object " << state << " " << id << "\n";
    using namespace dromozoa;
    new_userdata<object_t>(static_cast<lua_State*>(state), "dromozoa.web.bridge.object", id);
  }
}
