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

#ifndef DROMOZOA_WEB_THREAD_REFERENCE_HPP
#define DROMOZOA_WEB_THREAD_REFERENCE_HPP

#include "lua.hpp"
#include "noncopyable.hpp"

namespace dromozoa {
  class thread_reference : noncopyable {
  public:
    thread_reference();
    explicit thread_reference(lua_State*);
    thread_reference(thread_reference&&);
    ~thread_reference();
    thread_reference& operator=(thread_reference&&);
    lua_State* get() const;
    explicit operator bool() const;
  private:
    lua_State* thread_;
    int ref_;
    void unref();
    void reset();
  };
}

#endif
