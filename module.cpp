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

#include "common.hpp"
#include "lua.hpp"

namespace dromozoa {
  namespace {
    template <class T_key, class T_value>
    void preload_impl(lua_State* L, T_key&& key, T_value&& value) {
      lua_getglobal(L, "package");
      lua_getfield(L, -1, "preload");
      set_field(L, -1, std::forward<T_key>(key), std::forward<T_value>(value));
      lua_pop(L, 2);
    }
  }

  void initialize_core(lua_State*);
  void initialize_fetch(lua_State*);

  void preload_module(lua_State* L) {
    preload_impl(L, "dromozoa.web.core", function<initialize_core>());
    preload_impl(L, "dromozoa.web.fetch", function<initialize_fetch>());
  }
}
