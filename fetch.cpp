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
#include <memory>>
#include <set>
#include <utility>
#include "assert.hpp"
#include "common.hpp"
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"
#include "thread_reference.hpp"

namespace dromozoa {
  namespace {
    class fetch_t;

    class fetch_ref_t : noncopyable {
    public:
      explicit fetch_ref_t(fetch_t* ptr) : ptr_(ptr) {}

      fetch_t* get() const {
        if (!ptr_) {
          throw DROMOZOA_LOGIC_ERROR("attempt to use a detached dromozoa.web.fetch_ref");
        }
        return ptr_;
      }

      static void detach(fetch_ref_t* self) {
        self->ptr_ = nullptr;
      }

    private:
      fetch_t* ptr_;
    };

    std::set<emscripten_fetch_t*> fetches;

    class fetch_t : noncopyable {
    public:
      explicit fetch_t(thread_reference&& ref) : ref_(std::move(ref)), thread_(), fetch_() {
        if (lua_State* L = ref_.get()) {
          thread_ = lua_tothread(L, 2);
        }
      }

      ~fetch_t() {
        close();
      }

      void set_fetch(emscripten_fetch_t* fetch) {
        fetch_ = fetch;
        fetches.insert(fetch_);
      }

      emscripten_fetch_t* get_fetch() const {
        return fetch_;
      }

      void close() {
        if (fetch_) {
          emscripten_fetch_close(fetch_);
          fetches.erase(fetch_);
          fetch_ = nullptr;
        }
      }

      static void onsuccess(emscripten_fetch_t* fetch) {
        if (auto* self = cast(fetch)) {
          self->on_impl(self->ref_.get(), 1, fetch);
        }
      }

      static void onerror(emscripten_fetch_t* fetch) {
        if (auto* self = cast(fetch)) {
          self->on_impl(self->thread_, 1, fetch);
        }
      }

      static void onprogress(emscripten_fetch_t* fetch) {
        if (auto* self = cast(fetch)) {
          self->on_impl(self->ref_.get(), 3, fetch);
        }
      }

      static void onreadystatechange(emscripten_fetch_t* fetch) {
        if (auto* self = cast(fetch)) {
          self->on_impl(self->ref_.get(), 4, fetch);
        }
      }

    private:
      thread_reference ref_;
      lua_State* thread_;
      emscripten_fetch_t* fetch_;

      static fetch_t* cast(emscripten_fetch_t* fetch) {
        if (fetch && fetches.find(fetch) != fetches.end()) {
          return static_cast<fetch_t*>(fetch->userData);
        }
        return nullptr;
      }

      void on_impl(lua_State* L, int index, emscripten_fetch_t* fetch) {
        try {
          DROMOZOA_ASSERT(fetch_ == fetch);
          lua_pushvalue(L, index);
          std::unique_ptr<fetch_ref_t, decltype(&fetch_ref_t::detach)> guard(new_userdata<fetch_ref_t>(L, "dromozoa.web.fetch_ref", this), fetch_ref_t::detach);
          if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
          }
        } catch (...) {
          push_exception_queue();
        }
      }
    };

    fetch_t* check_fetch(lua_State* L, int index) {
      if (auto* ref = static_cast<fetch_ref_t*>(luaL_testudata(L, index, "dromozoa.web.fetch_ref"))) {
        return ref->get();
      }
      return static_cast<fetch_t*>(luaL_checkudata(L, index, "dromozoa.web.fetch"));
    }

    void impl_gc(lua_State* L) {
      check_fetch(L, 1)->~fetch_t();
    }

    void impl_call(lua_State* L) {
      emscripten_fetch_attr_t attr = {};
      emscripten_fetch_attr_init(&attr);

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

      // [1] function | onsuccess
      // [2] thread   | onerror
      // [3] function | onprogress
      // [4] function | onreadystatechange
      thread_reference ref = thread_reference(L);

      if (lua_getfield(L, 2, "onsuccess") != LUA_TNIL) {
        attr.onsuccess = fetch_t::onsuccess;
      }
      lua_xmove(L, ref.get(), 1);

      if (lua_getfield(L, 2, "onerror") != LUA_TNIL) {
        attr.onerror = fetch_t::onerror;
      }
      // [1] function | onerror
      lua_State* thread = lua_newthread(ref.get());
      lua_xmove(L, thread, 1);

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

      fetch_t* self = new_userdata<fetch_t>(L, "dromozoa.web.fetch", std::move(ref));
      attr.userData = self;
      emscripten_fetch_t* fetch = emscripten_fetch(&attr, url);
      self->set_fetch(fetch);
    }

    void impl_get_url(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->url);
    }

    void impl_get_data(lua_State* L) {
      fetch_t* self = check_fetch(L, 1);
      if (const char* data = self->get_fetch()->data) {
        lua_pushlstring(L, data, self->get_fetch()->numBytes);
      } else {
        lua_pushnil(L);
      }
    }

    void impl_get_data_pointer(lua_State* L) {
      fetch_t* self = check_fetch(L, 1);
      if (void* data = const_cast<char*>(self->get_fetch()->data)) {
        lua_pushlightuserdata(L, data);
      } else {
        lua_pushnil(L);
      }
    }

    void impl_get_num_bytes(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->numBytes);
    }

    void impl_get_total_bytes(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->totalBytes);
    }

    void impl_get_data_offset(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->dataOffset);
    }

    void impl_get_ready_state(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->readyState);
    }

    void impl_get_status(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->status);
    }

    void impl_get_status_text(lua_State* L) {
      push(L, check_fetch(L, 1)->get_fetch()->statusText);
    }

    void impl_close(lua_State* L) {
      check_fetch(L, 1)->close();
    }
  }

  void initialize_fetch(lua_State* L) {
    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.web.fetch_ref");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      lua_pop(L, 1);

      luaL_newmetatable(L, "dromozoa.web.fetch");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      set_field(L, -1, "__close", function<impl_close>());
      set_field(L, -1, "__gc", function<impl_gc>());
      lua_pop(L, 1);

      set_metafield(L, -1, "__call", function<impl_call>());

      set_field(L, -1, "get_url", function<impl_get_url>());
      set_field(L, -1, "get_data", function<impl_get_data>());
      set_field(L, -1, "get_data_pointer", function<impl_get_data_pointer>());
      set_field(L, -1, "get_num_bytes", function<impl_get_num_bytes>());
      set_field(L, -1, "get_total_bytes", function<impl_get_total_bytes>());
      set_field(L, -1, "get_data_offset", function<impl_get_data_offset>());
      set_field(L, -1, "get_ready_state", function<impl_get_ready_state>());
      set_field(L, -1, "get_status", function<impl_get_status>());
      set_field(L, -1, "get_status_text", function<impl_get_status_text>());
      set_field(L, -1, "close", function<impl_close>());

      set_field(L, -1, "LOAD_TO_MEMORY", EMSCRIPTEN_FETCH_LOAD_TO_MEMORY);
      set_field(L, -1, "STREAM_DATA", EMSCRIPTEN_FETCH_STREAM_DATA);
      set_field(L, -1, "PERSIST_FILE", EMSCRIPTEN_FETCH_PERSIST_FILE);
      set_field(L, -1, "APPEND", EMSCRIPTEN_FETCH_APPEND);
      set_field(L, -1, "REPLACE", EMSCRIPTEN_FETCH_REPLACE);
      set_field(L, -1, "NO_DOWNLOAD", EMSCRIPTEN_FETCH_NO_DOWNLOAD);
      set_field(L, -1, "SYNCHRONOUS", EMSCRIPTEN_FETCH_SYNCHRONOUS);
      set_field(L, -1, "WAITABLE", EMSCRIPTEN_FETCH_WAITABLE);

      set_field(L, -1, "UNSENT", 0);
      set_field(L, -1, "OPENED", 1);
      set_field(L, -1, "HEADERS_RECEIVED", 2);
      set_field(L, -1, "LOADING", 3);
      set_field(L, -1, "DONE", 4);
    }
    lua_setfield(L, -2, "fetch");
  }
}
