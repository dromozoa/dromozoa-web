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
#include "view.hpp"

namespace dromozoa {
  namespace {
    view_t* check_view(lua_State* L, int arg) {
      view_t* self = check_udata<view_t>(L, arg, "brigid.view");
      if (self->closed()) {
        luaL_argerror(L, arg, "attempt to use a closed brigid.view");
      }
      return self;
    }

    void impl_tostring(lua_State* L) {
      view_t* self = check_view(L, 1);
      lua_pushlstring(L, self->data(), self->size());
    }
  }

  view_t::view_t(const char* data, std::size_t size) : data_(data), size_(size) {}

  const char* view_t::data() const {
    return data_;
  }

  std::size_t view_t::size() const {
    return size_;
  }

  view_t::operator bool() const {
    return data_;
  }

  view_t* new_view(lua_State* L, const char* data, std::size_t size) {
    return new_userdata<view_t>(L, "dromozoa.view", data, size);
  }

  void initialize_view(lua_State* L) {
    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromoaoz.view");
      lua_pushvalue(L, -2);
      set_field(L, -2, "__tostring", function<impl_tostring>());

    }
    lua_setfield(L, -2, "view");
  }
}
