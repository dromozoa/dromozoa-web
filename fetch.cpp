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
      fetch_t(
          thread_reference&& ref,
          int onsuccess,
          int onerror,
          int onprogress,
          int onreadystatechange)
        : ref_(std::move(ref)),
          onsuccess_(onsuccess),
          onerror_(onerror),
          onprogress_(onprogress),
          onreadystatechange_(onreadystatechange),
          thread_(),
          fetch_() {
        if (lua_State* L = ref_.get()) {
          if (onerror_) {
            thread_ = lua_newthread(L);
          }
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
        if (fetch_) {
          emscripten_fetch_close(fetch_);
          fetch_ = nullptr;
        }
      }

      static void onsuccess(emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->onsuccess_, false, fetch);
        }
      }

      static void onerror(emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->onerror_, true, fetch);
        }
      }

      static void onprogress(emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->onprogress_, false, fetch);
        }
      }

      static void onreadystatechange(emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_t*>(fetch->userData)) {
          self->on(self->onreadystatechange_, false, fetch);
        }
      }

    private:
      thread_reference ref_;
      int onsuccess_;
      int onerror_;
      int onprogress_;
      int onreadystatechange_;
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

      void on(int on, bool iserror, emscripten_fetch_t* fetch) {
        try {
          if (!on) {
            return;
          }
          if (lua_State* L = ref_.get()) {
            if (iserror) {
              lua_pushvalue(L, on);
              lua_xmove(L, thread_, 1);
              if (lua_pcall(thread_, 0, 0, 0) != LUA_OK) {
                throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
              }
            } else {
              lua_pushvalue(L, on);
              if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
              }
            }
          }
        } catch (...) {
          push_exception_queue();
        }
      }
    };

    /*

typedef struct emscripten_fetch_attr_t {
  // 'POST', 'GET', etc.
  char requestMethod[32];

  // Custom data that can be tagged along the process.
  void *userData;

  void (*onsuccess)(struct emscripten_fetch_t *fetch);
  void (*onerror)(struct emscripten_fetch_t *fetch);
  void (*onprogress)(struct emscripten_fetch_t *fetch);
  void (*onreadystatechange)(struct emscripten_fetch_t *fetch);

  // EMSCRIPTEN_FETCH_* attributes
  uint32_t attributes;

  // Specifies the amount of time the request can take before failing due to a
  // timeout.
  unsigned long timeoutMSecs;

  // Indicates whether cross-site access control requests should be made using
  // credentials.
  EM_BOOL withCredentials;

  // Specifies the destination path in IndexedDB where to store the downloaded
  // content body. If this is empty, the transfer is not stored to IndexedDB at
  // all.  Note that this struct does not contain space to hold this string, it
  // only carries a pointer.
  // Calling emscripten_fetch() will make an internal copy of this string.
  const char *destinationPath;

  // Specifies the authentication username to use for the request, if necessary.
  // Note that this struct does not contain space to hold this string, it only
  // carries a pointer.
  // Calling emscripten_fetch() will make an internal copy of this string.
  const char *userName;

  // Specifies the authentication username to use for the request, if necessary.
  // Note that this struct does not contain space to hold this string, it only
  // carries a pointer.
  // Calling emscripten_fetch() will make an internal copy of this string.
  const char *password;

  // Points to an array of strings to pass custom headers to the request. This
  // array takes the form
  // {"key1", "value1", "key2", "value2", "key3", "value3", ..., 0 }; Note
  // especially that the array needs to be terminated with a null pointer.
  const char * const *requestHeaders;

  // Pass a custom MIME type here to force the browser to treat the received
  // data with the given type.
  const char *overriddenMimeType;

  // If non-zero, specifies a pointer to the data that is to be passed as the
  // body (payload) of the request that is being performed. Leave as zero if no
  // request body needs to be sent.  The memory pointed to by this field is
  // provided by the user, and needs to be valid throughout the duration of the
  // fetch operation. If passing a non-zero pointer into this field, make sure
  // to implement *both* the onsuccess and onerror handlers to be notified when
  // the fetch finishes to know when this memory block can be freed. Do not pass
  // a pointer to memory on the stack or other temporary area here.
  const char *requestData;

  // Specifies the length of the buffer pointed by 'requestData'. Leave as 0 if
  // no request body needs to be sent.
  size_t requestDataSize;
} emscripten_fetch_attr_t;


        {
          request_method = string?;
          onsuccess = function?;
          onerror = function?;
          onprogress = function?;
          onreadystatechange = function?;

          attributes = integer?;
          with_credentials = boolean?;
          destination_path = string?;
          username = string?;
          password = string?;
          request_headers = [string]?;
          overidden_mime_type = string?;
          request_data = string?;
        }



     */

    void impl_call(lua_State* L) {
      emscripten_fetch_attr_t attr;
      emscripten_fetch_attr_init(&attr);

      thread_reference ref;
      int ref_index = 0;
      int onsuccess = 0;
      int onerror = 0;
      int onprogress = 0;
      int onreadystatechange = 0;

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

      if (lua_getfield(L, 2, "attributes") != LUA_TNIL) {
        int result = 0;
        lua_Integer value = lua_tointegerx(L, -1, &result);
        if (!result) {
            throw DROMOZOA_RUNTIME_ERROR("field 'attributes' is not an integer");
        }
        attr.attributes = value;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onsuccess") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onsuccess = ++ref_index;
        attr.onsuccess = fetch_t::onsuccess;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onerror") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onerror = ++ref_index;
        attr.onerror = fetch_t::onerror;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onprogress") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onprogress = ++ref_index;
        attr.onprogress = fetch_t::onprogress;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onprogress") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onprogress = ++ref_index;
        attr.onreadystatechange = fetch_t::onreadystatechange;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onreadystatechange") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onreadystatechange = ++ref_index;
      }
      lua_pop(L, 1);

      const char* url = luaL_checkstring(L, 3);

      fetch_t* self = new_userdata<fetch_t>(L, "dromozoa.fetch", std::move(ref), onsuccess, onerror, onprogress, onreadystatechange);;
      attr.userData = self;

      emscripten_fetch_t* fetch = emscripten_fetch(&attr, url);
      self->set_fetch(fetch);
    }

    void impl_get_ready_state(lua_State* L) {
      fetch_t* self = static_cast<fetch_t*>(luaL_checkudata(L, 1, "dromozoa.fetch"));
      push(L, self->get_ready_state());
    }

    void impl_get_status(lua_State* L) {
      fetch_t* self = static_cast<fetch_t*>(luaL_checkudata(L, 1, "dromozoa.fetch"));
      push(L, self->get_status());
    }

    void impl_close(lua_State* L) {
      fetch_t* self = static_cast<fetch_t*>(luaL_checkudata(L, 1, "dromozoa.fetch"));
      self->close();
    }
  }

  void initialize_fetch(lua_State* L) {
    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.fetch");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
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
