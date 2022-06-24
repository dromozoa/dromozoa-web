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
#include <string_view>
#include "common.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"

/*
    document
      create_element
      create_document_fragment

    document_fragment
    element
    attr

    parent_node
      prepend
      append
      replace_children
      query_selector
      query_selector_all


    -- reference object

    javascript側にmapを作る

    ++id

    document { id }

    dromozoa_dom_data = new Map();

 */

namespace dromozoa {
  namespace {
    int object_id = 0;

    class object_t : noncopyable {
    public:
      explicit object_t() : id_(++object_id) {}

      ~object_t() {
        EM_ASM({
          dromozoa_web_dom.delete($0);
        }, id_);
      }

      int get_id() const {
        return id_;
      }

    private:
      int id_;
    };

    object_t* check_object(lua_State* L, int index) {
      stack_guard guard(L);
      const auto* name = luaL_typename(L, index);
      if (auto* self = static_cast<object_t*>(lua_touserdata(L, index))) {
        if (luaL_getmetafield(L, index, "__name") != LUA_TNIL) {
          if ((name = lua_tostring(L, -1))) {
            if (std::string_view(name).starts_with("dromozoa.web.dom.")) {
              return self;
            }
          }
        }
      }
      luaL_error(guard.release(), "dromozoa.web.dom.* expected, got %s", name);
      return nullptr;
    }

    void impl_gc(lua_State* L) {
      check_object(L, 1)->~object_t();
    }

    void impl_document_call(lua_State* L) {
      auto* self = new_userdata<object_t>(L, "dromozoa.web.dom.document");
      EM_ASM({
        dromozoa_web_dom.set($0, document);
      }, self->get_id());
    }

    void impl_query_selector(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* selectors = luaL_checkstring(L, 2);

      auto* that = new_userdata<object_t>(L, "dromozoa.web.dom.element");
      int result = EM_ASM_INT({
        const result = dromozoa_web_dom.get($1).querySelector(UTF8ToString($2));
        dromozoa_web_dom.set($0, result);
        return result ? 1 : 0;
      }, that->get_id(), self->get_id(), selectors);
      if (!result) {
        lua_pop(L, 1);
        lua_pushnil(L);
      }
    }

    void impl_query_selector_all(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* selectors = luaL_checkstring(L, 2);

      auto* that = new_userdata<object_t>(L, "dromozoa.web.dom.node_list");
      EM_ASM({
        dromozoa_web_dom.set($0, dromozoa_web_dom.get($1).querySelectorAll(UTF8ToString($2)));
      }, that->get_id(), self->get_id(), selectors);
    }

    void initialize_parent_node(lua_State* L) {
      set_field(L, -1, "query_selector", function<impl_query_selector>());
      set_field(L, -1, "query_selector_all", function<impl_query_selector_all>());
    }
  }

  void initialize_dom(lua_State* L) {
    EM_ASM({
      window.dromozoa_web_dom = new Map();
    });

    lua_newtable(L);
    {
      lua_newtable(L);
      {
        luaL_newmetatable(L, "dromozoa.web.dom.document");
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        set_field(L, -1, "__gc", function<impl_gc>());
        lua_pop(L, 1);

        set_metafield(L, -1, "__call", function<impl_document_call>());

        initialize_parent_node(L);
      }
      lua_setfield(L, -2, "document");

      lua_newtable(L);
      {
        luaL_newmetatable(L, "dromozoa.web.dom.element");
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        set_field(L, -1, "__gc", function<impl_gc>());
        lua_pop(L, 1);

        initialize_parent_node(L);
      }
      lua_setfield(L, -2, "element");

      lua_newtable(L);
      {
        luaL_newmetatable(L, "dromozoa.web.dom.node_list");
        lua_pushvalue(L, -2);
        lua_setfield(L, -2, "__index");
        set_field(L, -1, "__gc", function<impl_gc>());
        lua_pop(L, 1);
      }
      lua_setfield(L, -2, "node_list");
    }
  }
}
