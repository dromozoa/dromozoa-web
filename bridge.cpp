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
     */

    thread_reference ref;

    int object_id = 0;

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
            dromozoa_web_bridge.objects.delete($0);
          }, id_);
          id_ = 0;
        }
      }

    private:
      int id_ = 0;
    };

    object_t* check_object(lua_State* L, int index) {
      return static_cast<object_t*>(luaL_checkudata(L, index, "dromozoa.web.bridge.object"));
    }

    template <class T>
    std::unique_ptr<char, decltype(&std::free)> make_unique_cstr(T ptr) {
      return std::unique_ptr<char, decltype(&std::free)>(reinterpret_cast<char*>(ptr), std::free);
    }

    void impl_index(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* key = luaL_checkstring(L, 2);
      std::cout << "impl_index " << self << " " << key << "\n";

      int id = ++object_id;
      int type = EM_ASM_INT({
        const objects = dromozoa_web_bridge.objects;
        const result = objects.get($1)[UTF8ToString($2)];
        objects.set($0, result);
        return dromozoa_web_bridge.type_to_int(result);
      }, id, self->get_id(), key);

      if (type == 0 || type == 1) {
        lua_pushnil(L);
      } else if (type == 5) {
        auto result = EM_ASM_INT({
          return dromozoa_web_bridge.objects.get($0);
        }, id);
        push(L, result);
      } else if (type == 8) {
        auto result = make_unique_cstr(EM_ASM_INT({
          return dromozoa_web_bridge.malloc_utf8(dromozoa_web_bridge.objects.get($0));
        }, id));
        push(L, result.get());
      } else {
        new_userdata<object_t>(L, "dromozoa.web.bridge.object", id);
        return;
      }

      EM_ASM({
        dromozoa_web_bridge.objects.delete($0);
      }, id);
    }

    void impl_newindex(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* key = luaL_checkstring(L, 2);
      std::cout << "impl_newindex " << self << " " << key << "\n";

      switch (lua_type(L, 3)) {
        case LUA_TSTRING:
          EM_ASM({
            dromozoa_web_bridge.objects.get($0)[UTF8ToString($1)] = UTF8ToString($2);
          }, self->get_id(), key, lua_tostring(L, 3));
          break;
      }
    }

    void impl_close(lua_State* L) {
      auto* self = check_object(L, 1);
      std::cout << "impl_close " << self << "\n";
      self->close();
    }

    void impl_gc(lua_State* L) {
      auto* self = check_object(L, 1);
      std::cout << "impl_gc " << self << "\n";
      self->~object_t();
    }

    void impl_get_window(lua_State* L) {
      int id = ++object_id;
      EM_ASM({
        dromozoa_web_bridge.objects.set($0, window);
      }, id);
      new_userdata<object_t>(L, "dromozoa.web.bridge.object", id);
    }
  }

  void initialize_bridge(lua_State* L) {
    ref = thread_reference(L);

    EM_ASM({
      window.dromozoa_web_bridge = {};
      dromozoa_web_bridge.objects = new Map();
      dromozoa_web_bridge.type_to_int = function (v) {
        switch (typeof v) {
          case "undefined":
            return 0;
          case "object":
            if (v === null) {
              return 1;
            } else {
              return 2;
            }
          case "boolean":
            if (v === true) {
              return 3;
            } else {
              return 4;
            }
          case "number":
            if (Number.isInteger(v)) {
              return 5;
            } else {
              return 6;
            }
          case "bigint":
            return 7;
          case "string":
            return 8;
          case "symbol":
            return 9;
          case "function":
            return 10;
        }
        return 11;
      };
      dromozoa_web_bridge.malloc_utf8 = function (s) {
        const size = lengthBytesUTF8(s) + 1;
        const data = _malloc(size);
        stringToUTF8(s, data, size);
        return data;
      };
    });

    luaL_newmetatable(L, "dromozoa.web.bridge.object");
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
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
      std::cout << code << "\n";
      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        std::cerr << "cannot luaL_loadbuffer: " << lua_tostring(L, -1) << "\n";
        return;
      }
      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << "cannot lua_pcall: " << lua_tostring(L, -1) << "\n";
      }
    }
  }
}
