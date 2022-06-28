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
#include <cstring>
#include <exception>
#include <iostream>
#include <memory>
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"

namespace dromozoa {
  void preload_modules(lua_State*);

  class boot_t : noncopyable {
  public:
    boot_t() : state_(luaL_newstate()) {
      boot();
    }

    ~boot_t() {
      close();
    }

    static void each(void* data) {
      static_cast<boot_t*>(data)->each_impl();
    }

  private:
    lua_State* state_;

    void close() {
      if (state_) {
        lua_close(state_);
        state_ = nullptr;
      }
    }

    void boot() {
      try {
        if (auto* L = state_) {
          luaL_openlibs(L);
          preload_modules(L);

          static const char code[] =
          #include "boot.lua"
          ;

          if (luaL_loadbuffer(L, code, std::strlen(code), "boot.lua") != LUA_OK) {
            throw DROMOZOA_LOGIC_ERROR("cannot luaL_loadbuffer: ", lua_tostring(L, -1));
          }
          if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
          }
        }
      } catch (...) {
        push_exception_queue();
      }
    }

    void check_exception_queue() {
      for (int i = 0; ; ++i) {
        if (std::exception_ptr eptr = pop_exception_queue()) {
          try {
            std::rethrow_exception(eptr);
          } catch (const std::exception& e) {
            std::cerr << "exception " << e.what() << "\n";
          } catch (...) {
            std::cerr << "unknown exception\n";
          }
        } else {
          if (i > 0) {
            close();
          }
          return;
        }
      }
    }

    void each_impl() {
      check_exception_queue();

      try {
        if (auto* L = state_) {
          lua_pushvalue(L, -1);
          if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            throw DROMOZOA_LOGIC_ERROR("canot lua_pcall: ", lua_tostring(L, -1));
          }
        }
      } catch (...) {
        push_exception_queue();
      }

      check_exception_queue();
    }
  };
}

int main() {
  auto boot = std::make_unique<dromozoa::boot_t>();
  emscripten_set_main_loop_arg(dromozoa::boot_t::each, boot.release(), 30, true);
  return 0;
}
