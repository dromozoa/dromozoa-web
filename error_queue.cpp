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

#include <deque>
#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include "common.hpp"
#include "lua.hpp"
#include "error_queue.hpp"

namespace dromozoa {
  namespace {
    std::deque<std::exception_ptr> error_queue;

    std::optional<std::string> pop_error_queue() {
      if (error_queue.empty()) {
        return std::nullopt;
      }

      try {
        auto e = error_queue.front();
        error_queue.pop_front();
        std::rethrow_exception(e);
      } catch (const std::exception& e) {
        return e.what();
      } catch (...) {}
      return "unknown exception";
    }

    void impl_pop_error_queue(lua_State* L) {
      if (auto e = pop_error_queue()) {
        lua_pushstring(L, e->c_str());
      } else {
        lua_pushnil(L);
      }
    }

    void impl_get_error_queue(lua_State* L) {
      if (error_queue.empty()) {
        lua_pushboolean(L, true);
        return;
      }

      const char* sep = luaL_optstring(L, 1, "\n\t");
      std::ostringstream out;
      for (int i = 0; auto e = pop_error_queue(); ++i) {
        if (i > 0) {
          out << sep;
        }
        out << *e;
      }
      lua_pushnil(L);
      push(L, out.str());
    }
  }

  void push_error_queue() {
    error_queue.emplace_back(std::current_exception());
  }

  void initialize_error_queue(lua_State* L) {
    set_field(L, -1, "pop_error_queue", function<impl_pop_error_queue>());
    set_field(L, -1, "get_error_queue", function<impl_get_error_queue>());
  }
}
