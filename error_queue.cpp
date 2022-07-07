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
#include <optional>
#include <sstream>
#include <string>
#include "common.hpp"
#include "lua.hpp"
#include "error_queue.hpp"

namespace dromozoa {
  namespace {
    std::deque<std::string> error_queue;

    void impl_pop_error_queue(lua_State* L) {
      if (error_queue.empty()) {
        lua_pushnil(L);
      } else {
        auto e = error_queue.front();
        error_queue.pop_front();
        push(L, e);
      }
    }

    void impl_get_error_queue(lua_State* L) {
      if (error_queue.empty()) {
        lua_pushboolean(L, true);
        return;
      }

      std::deque<std::string> queue;
      std::swap(queue, error_queue);

      const char* sep = luaL_optstring(L, 1, "\n\t");
      int i = 0;
      std::ostringstream out;
      for (const auto& e : queue) {
        if (i > 0) {
          out << sep;
        }
        ++i;
        out << e;
      }

      luaL_pushfail(L);
      push(L, out.str());
    }
  }

  void push_error_queue(const std::string& e) {
    error_queue.emplace_back(e);
  }

  void initialize_error_queue(lua_State* L) {
    set_field(L, -1, "pop_error_queue", function<impl_pop_error_queue>());
    set_field(L, -1, "get_error_queue", function<impl_get_error_queue>());
  }
}
