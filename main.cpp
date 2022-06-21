// Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
//
// This file is part of dromozoa-web.
//
// dromozoa-png is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// dromozoa-png is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with dromozoa-png.  If not, see <http://www.gnu.org/licenses/>.

#include <emscripten.h>
#include <emscripten/fetch.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <iostream>
#include <exception>
#include "error.hpp"

class context_t {
public:
  context_t() : state_() {
    state_ = luaL_newstate();
    luaL_openlibs(state_);
  }

  ~context_t() {
    lua_close(state_);
    state_ = nullptr;
  }

  void load();
  void each();

private:
  lua_State* state_;
};

void download_succeeded(emscripten_fetch_t *fetch) {
  std::cout << "download_succeeded " << fetch->numBytes << " " << fetch->url << "\n";
  emscripten_fetch_close(fetch);
}

void download_failed(emscripten_fetch_t *fetch) {
  std::cout << "download_succeeded " << fetch->status << " " << fetch->url << "\n";
  emscripten_fetch_close(fetch);
}

void context_t::load() {
  lua_State* L = state_;

  static const char code[] =
  #include "main.lua"
  ;

  if (luaL_loadbuffer(L, code, strlen(code), "=(load)") != 0) {
    throw DROMOZOA_LOGIC_ERROR("could not luaL_loadbuffer: ", lua_tostring(L, -1));
  }
  lua_pcall(L, 0, 1, 0);

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  memcpy(attr.requestMethod, "GET", 4);
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
  attr.onsuccess = download_succeeded;
  attr.onerror = download_failed;
  emscripten_fetch(&attr, "README.md");

  std::cout << "load done\n";
}

void context_t::each() {
  lua_State* L = state_;
  if (lua_type(L, -1) == LUA_TFUNCTION) {
    lua_pushvalue(L, -1);
    lua_pcall(L, 0, 0, 0);
  }
}

void each(void* data) {
  context_t* context = static_cast<context_t*>(data);
  context->each();
}

int main() {
  context_t* context = new context_t();
  try {
    context->load();
    emscripten_set_main_loop_arg(each, context, 30, true);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
  }
  return 0;
}
