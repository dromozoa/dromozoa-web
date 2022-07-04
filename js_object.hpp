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

#ifndef DROMOZOA_WEB_JS_OBJECT_HPP
#define DROMOZOA_WEB_JS_OBJECT_HPP

#include "lua.hpp"
#include "noncopyable.hpp"

namespace dromozoa {
  class js_object : noncopyable {
  public:
    static constexpr char NAME[] = "dromozoa.web.js_object";
    explicit js_object(int ref) : ref_(ref) {}
    ~js_object() { close(); }
    int get() const { return ref_; }
    void close() noexcept;
  private:
    int ref_;
  };

  void initialize_js_object(lua_State*);
}

#endif
