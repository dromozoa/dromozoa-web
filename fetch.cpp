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
    };

    class fetch_callback_t : noncopyable {
    public:
      fetch_callback_t(
          thread_reference&& ref,
          int onsuccess,
          int onerror,
          int onprogress)
        : ref_(std::move(ref)),
          onsuccess_(onsuccess),
          onerror_(onerror),
          onprogress_(onprogress) {}

      static void onsuccess(struct emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_callback_t*>(fetch->userData)) {
          self->onsuccess_impl(fetch);
        }
      }

      static void onerror(struct emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_callback_t*>(fetch->userData)) {
          self->onerror_impl(fetch);
        }
      }

      static void onprogress(struct emscripten_fetch_t* fetch) {
        if (auto* self = static_cast<fetch_callback_t*>(fetch->userData)) {
          self->onprogress_impl(fetch);
        }
      }

    private:
      thread_reference ref_;
      int onsuccess_;
      int onerror_;
      int onprogress_;

      void onsuccess_impl(struct emscripten_fetch_t* fetch) {
        try {
          if (onsuccess_) {
            if (lua_State* L = ref_.get()) {
              lua_pushvalue(L, onsuccess_);
              if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
              }
            }
          }
        } catch (...) {
          push_exception_queue();
        }
      }

      void onerror_impl(struct emscripten_fetch_t* fetch) {
        try {
          if (onerror_) {
            if (lua_State* L = ref_.get()) {
              lua_pushvalue(L, onerror_);
              if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                throw DROMOZOA_RUNTIME_ERROR(lua_tostring(L, -1));
              }
            }
          }
        } catch (...) {
          push_exception_queue();
        }
      }

      void onprogress_impl(struct emscripten_fetch_t* fetch) {
        try {
          if (onprogress_) {
            if (lua_State* L = ref_.get()) {
              lua_pushvalue(L, onprogress_);
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
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onerror") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onerror = ++ref_index;
      }
      lua_pop(L, 1);

      if (lua_getfield(L, 2, "onprogress") != LUA_TNIL) {
        if (!ref) {
          ref = thread_reference(L);
        }
        lua_pushvalue(L, -1);
        lua_xmove(L, ref.get(), 1);
        onprogress = ++ref_index;
      }
      lua_pop(L, 1);

      const char* url = luaL_checkstring(L, 3);

      // あとでラップする
      std::unique_ptr<fetch_callback_t> fetch_callback;
      if (ref) {
        fetch_callback = std::make_unique<fetch_callback_t>(std::move(ref), onsuccess, onerror, onprogress);
        attr.userData = fetch_callback.release();
        attr.onsuccess = fetch_callback_t::onsuccess;
        attr.onerror = fetch_callback_t::onerror;
        attr.onprogress = fetch_callback_t::onprogress;
      }

      emscripten_fetch(&attr, url);
    }

  }

  void initialize_fetch(lua_State* L) {
    lua_newtable(L);
    {
      set_metafield(L, -1, "__call", function<impl_call>());

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
