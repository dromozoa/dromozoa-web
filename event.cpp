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

#include <emscripten/html5.h>
#include <sstream>
#include "common.hpp"
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"
#include "thread_reference.hpp"

/*
  イベントハンドラ全体でひとつのthreadを使う
  コールバックは、REGISTRY["dromozoa.web.event"]テーブルに保存する
  メタテーブルと名前衝突しないように注意

  スタックにテーブルを用意して、文字列と関数をおいておく。
  key = { object, function };


  {
    key = { function, key };



  }

 */

namespace dromozoa {
  namespace {
    thread_reference ref;

    std::string make_callback_key(const char* target, int event_type) {
      std::ostringstream out;
      out << target << "/" << event_type;
      return out.str();
    }

    struct event_t {
    public:
      explicit event_t(const std::string& key) : key_(key) {}

      static EM_BOOL callback(int event_type, const EmscriptenMouseEvent* event, void* data) {
        if (auto* self = static_cast<event_t*>(data)) {
          return self->callback_impl(event_type, event);
        }
        return false;
      }

    private:
      std::string key_;

      bool callback_impl(int event_type, const EmscriptenMouseEvent* event) {
        try {
          if (lua_State* L = ref.get()) {
            stack_guard guard(L);
            push(L, key_);
            if (lua_gettable(L, 1) != LUA_TNIL) {
              if (lua_geti(L, -1, 1) != LUA_TNIL) {
                if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
                  throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
                }
                if (lua_isboolean(L, -1) && !lua_toboolean(L, -1)) {
                  return false;
                } else {
                  return true;
                }
              }
            }
          }
        } catch (...) {
          push_exception_queue();
        }
        return false;
      }
    };

    void impl_gc(lua_State* L) {
      static_cast<event_t*>(luaL_checkudata(L, 1, "dromozoa.web.event"))->~event_t();
    }

    void impl_set_click_callback(lua_State* L) {
      const auto* target = luaL_checkstring(L, 1);
      auto use_capture = lua_toboolean(L, 2);

      std::string key = make_callback_key(target, EMSCRIPTEN_EVENT_CLICK);

      event_t* self = nullptr;

      lua_pushvalue(ref.get(), 1);
      push(ref.get(), key);
      if (lua_isnil(L, 3)) {
        // nil
        lua_pushnil(ref.get());
      } else {
        // { string, userdata }
        lua_newtable(ref.get());
        lua_pushvalue(L, 3);
        lua_xmove(L, ref.get(), 1);
        lua_seti(ref.get(), -2, 1);
        self = new_userdata<event_t>(ref.get(), "dromozoa.web.event", key);
        lua_seti(ref.get(), -2, 2);
      }
      lua_settable(ref.get(), -3);
      lua_pop(ref.get(), 1);

      // 第三引数がnilだったら、削除する
      if (lua_isnil(L, 3)) {
        emscripten_set_click_callback(target, self, use_capture, nullptr);
      } else {
        emscripten_set_click_callback(target, self, use_capture, event_t::callback);
      }
    }
  }

  void initialize_event(lua_State* L) {
    ref = thread_reference(L);
    lua_newtable(ref.get());

    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.web.event");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      set_field(L, -1, "__gc", function<impl_gc>());
      lua_pop(L, 1);

      set_field(L, -1, "set_click_callback", function<impl_set_click_callback>());
    }
  }
}
