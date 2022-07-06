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

#ifndef DROMOZOA_WEB_STACK_GUARD_HPP
#define DROMOZOA_WEB_STACK_GUARD_HPP

#include "lua.hpp"
#include "noncopyable.hpp"

namespace dromozoa {
  class stack_guard : noncopyable {
  public:
    explicit stack_guard(lua_State* L) : state_(L), top_(lua_gettop(L)) {}

    ~stack_guard() {
      if (state_) {
        lua_settop(state_, top_);
      }
    }

    lua_State* release() {
      auto* state = state_;
      state_ = nullptr;
      return state;
    }

  private:
    lua_State* state_;
    int top_;
  };
}

#endif
