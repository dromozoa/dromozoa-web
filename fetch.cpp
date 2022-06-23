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

#include <string.h>
#include <emscripten/fetch.h>
#include <iostream>
#include <utility>
#include "common.hpp"
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"
#include "thread_reference.hpp"

namespace dromozoa {
  namespace {
    class fetch_t : noncopyable {
    public:
      fetch_t(thread_reference&& ref) : ref_(std::move(ref)), thread_(), fetch_() {
        if (lua_State* L = ref_.get()) {
          thread_ = lua_tothread(L, 2);
        }
      }

      void set_fetch(emscripten_fetch_t* fetch) {
        fetch_ = fetch;
      }

      unsigned short get_ready_state() const {
        return fetch_->readyState;
      }

      unsigned short get_status() const {
        return fetch_->status;
      }

      void close() {
        // emscripten_fetch_closeが戻った後もコールバックは呼ばれうる。
        if (fetch_) {
          std::cout << "closing\n";
          emscripten_fetch_close(fetch_);
          std::cout << "closed\n";
          fetch_->userData = nullptr;
          fetch_ = nullptr;
        }
      }

      static void onsuccess(emscripten_fetch_t* fetch) {
        std::cout << "onsuccess\n";
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->ref_.get(), 1, fetch);
        }
      }

      static void onerror(emscripten_fetch_t* fetch) {
        std::cout << "onerror\n";
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->thread_, 1, fetch);
        }
      }

      static void onprogress(emscripten_fetch_t* fetch) {
        std::cout << "onprogress\n";
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->ref_.get(), 3, fetch);
        }
      }

      static void onreadystatechange(emscripten_fetch_t* fetch) {
        std::cout << "onreadystatechange\n";
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->ref_.get(), 4, fetch);
        }
      }

    private:
      // 1 : function: onsuccess
      // 2 : thread:   onerror
      // 3 : function: onprogress
      // 4 : function: onreadystatechange
      thread_reference ref_;
      // 1 : function: onerror
      lua_State* thread_;
      emscripten_fetch_t* fetch_;

      // emscripten_fetch_closeはキャンセルに使える（onprogressからよんだり？）
      // emscripten_fetch_closeが返るまでのあいだに、onerrorが呼ばれる
      // emscripten_fetch_closeを呼んだあと、fetch_callback_tをdeleteするのはOK
      //   再入する可能性がある
      //     onerrorの処理だけ、別のrefにする？
      //     再入を検知して、そのときだけ、スレッドをつくる？
      //     毎回、lua_newthreadする？
      // emscripten_fetch_closeを呼ばなかったら？
      // emscripten_fetch_tをLuaに保存しておくか

      void on(lua_State* L, int index, emscripten_fetch_t* fetch) {
        try {
          lua_pushvalue(L, index);
          if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
          }
        } catch (...) {
          push_exception_queue();
        }
      }
    };

    fetch_t* check_fetch(lua_State* L, int index) {
      return static_cast<fetch_t*>(luaL_checkudata(L, index, "dromozoa.fetch"));
    }

    void impl_gc(lua_State* L) {
      check_fetch(L, 1)->~fetch_t();
    }

    void impl_call(lua_State* L) {
      emscripten_fetch_attr_t attr;
      emscripten_fetch_attr_init(&attr);
      thread_reference ref = thread_reference(L);

      if (lua_getfield(L, 2, "request_method") != LUA_TNIL) {
        size_t size = 0;
        if (const char* data = lua_tolstring(L, -1, &size)) {
          if (size >= sizeof(attr.requestMethod)) {
            throw DROMOZOA_RUNTIME_ERROR("field 'request_method' is too long");
          }
          memcpy(attr.requestMethod, data, size);
        }
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onsuccess") != LUA_TNIL) {
        attr.onsuccess = fetch_t::onsuccess;
      }
      lua_xmove(L, ref.get(), 1);

      if (lua_getfield(L, 2, "onerror") != LUA_TNIL) {
        lua_State* thread = lua_newthread(ref.get());
        lua_xmove(L, thread, 1);
        attr.onerror = fetch_t::onerror;
      } else {
        lua_pop(L, 1);
        lua_pushnil(ref.get());
      }

      if (lua_getfield(L, 2, "onprogress") != LUA_TNIL) {
        attr.onprogress = fetch_t::onprogress;
      }
      lua_xmove(L, ref.get(), 1);

      if (lua_getfield(L, 2, "onreadystatechange") != LUA_TNIL) {
        attr.onreadystatechange = fetch_t::onreadystatechange;
      }
      lua_xmove(L, ref.get(), 1);

      if (lua_getfield(L, 2, "attributes") != LUA_TNIL) {
        int is_integer = 0;
        lua_Integer value = lua_tointegerx(L, -1, &is_integer);
        if (!is_integer) {
            throw DROMOZOA_RUNTIME_ERROR("field 'attributes' is not an integer");
        }
        attr.attributes = value;
      }
      lua_pop(L, 1);

      const char* url = luaL_checkstring(L, 3);

      fetch_t* self = new_userdata<fetch_t>(L, "dromozoa.fetch", std::move(ref));
      attr.userData = self;
      emscripten_fetch_t* fetch = emscripten_fetch(&attr, url);
      self->set_fetch(fetch);
    }

    void impl_get_ready_state(lua_State* L) {
      push(L, check_fetch(L, 1)->get_ready_state());
    }

    void impl_get_status(lua_State* L) {
      push(L, check_fetch(L, 1)->get_status());
    }

    void impl_close(lua_State* L) {
      check_fetch(L, 1)->close();
    }
  }

  void initialize_fetch(lua_State* L) {
    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.fetch");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      set_field(L, -2, "__gc", function<impl_gc>());
      lua_pop(L, 1);

      set_metafield(L, -1, "__call", function<impl_call>());

      set_field(L, -1, "get_ready_state", function<impl_get_ready_state>());
      set_field(L, -1, "get_status", function<impl_get_status>());
      set_field(L, -1, "close", function<impl_close>());

      set_field(L, -1, "LOAD_TO_MEMORY", EMSCRIPTEN_FETCH_LOAD_TO_MEMORY);
      set_field(L, -1, "STREAM_DATA", EMSCRIPTEN_FETCH_STREAM_DATA);
      set_field(L, -1, "PERSIST_FILE", EMSCRIPTEN_FETCH_PERSIST_FILE);
      set_field(L, -1, "APPEND", EMSCRIPTEN_FETCH_APPEND);
      set_field(L, -1, "REPLACE", EMSCRIPTEN_FETCH_REPLACE);
      set_field(L, -1, "NO_DOWNLOAD", EMSCRIPTEN_FETCH_NO_DOWNLOAD);
      set_field(L, -1, "SYNCHRONOUS", EMSCRIPTEN_FETCH_SYNCHRONOUS);
      set_field(L, -1, "WAITABLE", EMSCRIPTEN_FETCH_WAITABLE);
    }
    lua_setfield(L, -2, "fetch");
  }
}
