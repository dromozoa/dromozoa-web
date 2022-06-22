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

#include <emscripten/fetch.h>
#include <iostream>
#include "common.hpp"
#include "lua.hpp"

namespace dromozoa {
  namespace {
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
      // 1: attr
      // 2: url

      std::cout << "fetch!\n";





    }
  }

  void initialize_fetch(lua_State* L) {
    lua_newtable(L);
    {
      // decltype(function<impl_call>())::set_metafield(L, -1, "__call");
      set_metafield(L, -1, "__call", function<impl_call>());
      set_metafield(L, -1, "test", 42);

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
