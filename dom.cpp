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
    int ref_id = 0;

    class ref_t : noncopyable {
    public:
      explicit ref_t() : id_(++ref_id) {}

      ~ref_t() {
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

    ref_t* check_ref(lua_State* L, int index) {
      return static_cast<ref_t*>(luaL_checkudata(L, index, "dromozoa.web.dom"));
    }

    void impl_gc(lua_State* L) {
      check_ref(L, 1)->~ref_t();
    }

    void impl_get_document(lua_State* L) {
      ref_t* self = new_userdata<ref_t>(L, "dromozoa.web.dom");
      EM_ASM({
        dromozoa_web_dom.set($0, document);
      }, self->get_id());
    }
  }

  void initialize_dom(lua_State* L) {
    EM_ASM({
      window.dromozoa_web_dom = new Map();
    });

    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.web.dom");
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      set_field(L, -1, "__gc", function<impl_gc>());
      lua_pop(L, 1);

      set_field(L, -1, "get_document", function<impl_get_document>());
    }
  }
}
