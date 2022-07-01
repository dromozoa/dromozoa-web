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
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"

namespace dromozoa {
  namespace {
    void impl_run_script(lua_State* L) {
      const auto* script = luaL_checkstring(L, 1);
      emscripten_run_script(script);
    }

    void impl_run_script_int(lua_State* L) {
      const auto* script = luaL_checkstring(L, 1);
      push(L, emscripten_run_script_int(script));
    }

    void impl_run_script_string(lua_State* L) {
      const auto* script = luaL_checkstring(L, 1);
      push(L, emscripten_run_script_string(script));
    }

    void impl_get_device_pixel_ratio(lua_State* L) {
      push(L, emscripten_get_device_pixel_ratio());
    }

    void impl_get_window_title(lua_State* L) {
      push(L, emscripten_get_window_title());
    }

    void impl_set_window_title(lua_State* L) {
      const auto* title = luaL_checkstring(L, 1);
      emscripten_set_window_title(title);
    }

    void impl_get_screen_size(lua_State* L) {
      int width = 0;
      int height = 0;
      emscripten_get_screen_size(&width, &height);
      push(L, width);
      push(L, height);
    }

    void impl_hide_mouse(lua_State*) {
      emscripten_hide_mouse();
    }

    void impl_get_now(lua_State* L) {
      push(L, emscripten_get_now());
    }

    void impl_random(lua_State* L) {
      push(L, emscripten_random());
    }

    void impl_exit(lua_State*) {
      throw DROMOZOA_LOGIC_ERROR("__exit__");
    }
  }

  void initialize_core(lua_State* L) {
    lua_newtable(L);
    {
      set_field(L, -1, "run_script", function<impl_run_script>());
      set_field(L, -1, "run_script_int", function<impl_run_script_int>());
      set_field(L, -1, "run_script_string", function<impl_run_script_string>());
      set_field(L, -1, "get_device_pixel_ratio", function<impl_get_device_pixel_ratio>());
      set_field(L, -1, "get_window_title", function<impl_get_window_title>());
      set_field(L, -1, "set_window_title", function<impl_set_window_title>());
      set_field(L, -1, "get_screen_size", function<impl_get_screen_size>());
      set_field(L, -1, "hide_mouse", function<impl_hide_mouse>());
      set_field(L, -1, "get_now", function<impl_get_now>());
      set_field(L, -1, "random", function<impl_random>());
      set_field(L, -1, "exit", function<impl_exit>());
    }
  }
}
