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
#include <chrono>
#include <iostream>
#include <exception>
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"

extern "C" int luaopen_dromozoa(lua_State*);

class context_t {
public:
  context_t() : state_(luaL_newstate()) {}

  ~context_t() {
    close();
  }

  void load();
  void each();

private:
  lua_State* state_;

  void close() {
    if (state_) {
      lua_close(state_);
      state_ = nullptr;
    }
  }
};

void context_t::load() {
  try {
    if (lua_State* L = state_) {
      luaL_openlibs(L);

      lua_getglobal(L, "package");
      lua_getfield(L, -1, "preload");
      lua_pushcfunction(L, luaopen_dromozoa);
      lua_setfield(L, -2, "dromozoa");
      lua_pop(L, 2);

      static const char code[] =
      #include "main.lua"
      ;

      if (luaL_loadbuffer(L, code, strlen(code), "=(load)") != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot luaL_loadbuffer: ", lua_tostring(L, -1));
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
      }
    }
  } catch (...) {
    dromozoa::push_exception_queue();
  }
}

void context_t::each() {
  bool error = false;

  while (std::exception_ptr eptr = dromozoa::pop_exception_queue()) {
    error = true;
    try {
      std::rethrow_exception(eptr);
    } catch (const std::runtime_error& e) {
      std::cerr << "std::runtime_error " << e.what() << "\n";
    } catch (const std::exception& e) {
      std::cerr << "std::exception " << e.what() << "\n";
    } catch (...) {
      std::cerr << "unknown exception\n";
    }
  }

  if (error) {
    close();
  }

  try {
    if (auto* L = state_) {
      lua_pushvalue(L, -1);
      if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("canot lua_pcall: ", lua_tostring(L, -1));
      }
    }
  } catch (...) {
    dromozoa::push_exception_queue();
  }
}

void each(void* data) {
  auto* context = static_cast<context_t*>(data);
  context->each();
}

int main() {
  auto* context = new context_t();
  context->load();
  emscripten_set_main_loop_arg(each, context, 30, true);
  return 0;
}
