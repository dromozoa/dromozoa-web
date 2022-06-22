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

#include "lua.hpp"
#include "thread_reference.hpp"

namespace dromozoa {
  thread_reference::thread_reference()
    : thread_(),
      ref_(LUA_NOREF) {}

  thread_reference::thread_reference(lua_State* L)
    : thread_(),
      ref_(LUA_NOREF) {
    thread_ = lua_newthread(L);
    ref_ = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  thread_reference::thread_reference(thread_reference&& that)
    : thread_(that.thread_),
      ref_(that.ref_) {
    that.reset();
  }

  thread_reference::~thread_reference() {
    unref();
  }

  thread_reference& thread_reference::operator=(thread_reference&& that) {
    if (this != &that) {
      unref();
      thread_ = that.thread_;
      ref_ = that.ref_;
      that.reset();
    }
    return *this;
  }

  lua_State* thread_reference::get() const {
    return thread_;
  }

  thread_reference::operator bool() const {
    return thread_;
  }

  void thread_reference::unref() {
    if (lua_State* L = thread_) {
      luaL_unref(L, LUA_REGISTRYINDEX, ref_);
      reset();
    }
  }

  void thread_reference::reset() {
    thread_ = nullptr;
    ref_ = LUA_NOREF;
  }
}
