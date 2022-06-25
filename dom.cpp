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
#include <cstdlib>
#include <memory>
#include <string_view>
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"

/*
  Mixin NonElementParentNode
  Mixin DocumentOrShadowRoot
  Mixin ParentNode
  Mixin NonDocumentTypeChildNode
  Mixin ChildNode
  Mixin Slottable

  Mixin NonElementParentNode
    Document includes NonElementParentNode;
    DocumentFragment includes NonElementParentNode;
  Mixin DocumentOrShadowRoot
    Document includes DocumentOrShadowRoot;
    ShadowRoot includes DocumentOrShadowRoot;
  Mixin ParentNode
    Document includes ParentNode;
    DocumentFragment includes ParentNode;
    Element includes ParentNode;
  Mixin NonDocumentTypeChildNode
    Element includes NonDocumentTypeChildNode;
    CharacterData includes NonDocumentTypeChildNode;
  Mixin ChildNode
    DocumentType includes ChildNode;
    Element includes ChildNode;
    CharacterData includes ChildNode;
  Mixin Slottable
    Element includes Slottable;
    Text includes Slottable;



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
      explicit object_t(int id) : id_(id) {}

      ~object_t() {
        close();
      }

      int get_id() const {
        return id_;
      }

      void close() {
        if (id_) {
          EM_ASM({
            dromozoa_web_dom.delete($0);
          }, id_);
          id_ = 0;
        }
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

    const char* node_type_to_name(int node_type) {
      switch (node_type) {
        case 1: return "dromozoa.web.dom.element";
        case 9: return "dromozoa.web.dom.document";
        // case 11: return "dromozoa.web.dom.document_fragment";
        default: return "dromozoa.web.dom.node";
      }
    }

    template <typename T>
    std::unique_ptr<char, decltype(&std::free)> make_unique_cstr(T ptr) {
      return std::unique_ptr<char, decltype(&std::free)>(reinterpret_cast<char*>(ptr), std::free);
    }

    ///////////////////////////////////////////////////////////////////////

    void impl_close(lua_State* L) {
      check_object(L, 1)->close();
    }

    void impl_gc(lua_State* L) {
      check_object(L, 1)->~object_t();
    }

    void initialize_interface(lua_State* L, const char* name) {
      luaL_newmetatable(L, name);
      lua_pushvalue(L, -2);
      lua_setfield(L, -2, "__index");
      set_field(L, -1, "__close", function<impl_close>());
      set_field(L, -1, "__gc", function<impl_gc>());
      lua_pop(L, 1);
    }

    ///////////////////////////////////////////////////////////////////////

    void impl_document_call(lua_State* L) {
      auto id = ++object_id;
      EM_ASM({
        dromozoa_web_dom.set($0, document);
      }, id);
      new_userdata<object_t>(L, "dromozoa.web.dom.document", id);
    }

    ///////////////////////////////////////////////////////////////////////

    void impl_get_attribute_names(lua_State* L) {
      auto* self = check_object(L, 1);

      auto id = ++object_id;
      auto result = EM_ASM_INT({
        const result = dromozoa_web_dom.get($1).getAttributeNames();
        dromozoa_web_dom.set($0, result);
        return result.length;
      }, id, self->get_id());

      lua_newtable(L);
      for (int i = 0; i < result; ++i) {
        auto result = make_unique_cstr(EM_ASM_INT({
          const result = dromozoa_web_dom.get($0)[$1];
          if (result) {
            const size = lengthBytesUTF8(result) + 1;
            const data = _malloc(size);
            stringToUTF8(result, data, size);
            return data;
          } else {
            return 0;
          }
        }, id, i));
        if (result) {
          push(L, result.get());
        } else {
          lua_pushnil(L);
        }
        lua_seti(L, -2, i + 1);
      }

      EM_ASM({
        dromozoa_web_dom.delete($0);
      }, id);
    }

    void impl_get_attribute(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* name = luaL_checkstring(L, 2);

      auto result = make_unique_cstr(EM_ASM_INT({
        const result = dromozoa_web_dom.get($0).getAttribute(UTF8ToString($1));
        if (result) {
          const size = lengthBytesUTF8(result) + 1;
          const data = _malloc(size);
          stringToUTF8(result, data, size);
          return data;
        } else {
          return 0;
        }
      }, self->get_id(), name));
      if (result) {
        push(L, result.get());
      } else {
        lua_pushnil(L);
      }
    }

    void impl_set_attribute(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* name = luaL_checkstring(L, 2);
      const auto* value = luaL_checkstring(L, 3);

      EM_ASM({
        dromozoa_web_dom.get($0).setAttribute(UTF8ToString($1), UTF8ToString($2));
      }, self->get_id(), name, value);
    }

    void impl_remove_attribute(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* name = luaL_checkstring(L, 2);

      EM_ASM({
        dromozoa_web_dom.get($0).removeAttribute(UTF8ToString($1));
      }, self->get_id(), name);
    }

    ///////////////////////////////////////////////////////////////////////

    void impl_query_selector(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* selectors = luaL_checkstring(L, 2);

      auto id = ++object_id;
      auto result = EM_ASM_INT({
        const result = dromozoa_web_dom.get($1).querySelector(UTF8ToString($2));
        if (result) {
          dromozoa_web_dom.set($0, result);
          return 1;
        } else {
          return 0;
        }
      }, id, self->get_id(), selectors);

      if (result) {
        new_userdata<object_t>(L, "dromozoa.web.dom.element", id);
      } else {
        lua_pushnil(L);
      }
    }

    void impl_query_selector_all(lua_State* L) {
      auto* self = check_object(L, 1);
      const auto* selectors = luaL_checkstring(L, 2);

      auto id = ++object_id;
      auto result = EM_ASM_INT({
        const result = dromozoa_web_dom.get($1).querySelectorAll(UTF8ToString($2));
        dromozoa_web_dom.set($0, result);
        return result.length;
      }, id, self->get_id(), selectors);

      lua_newtable(L);
      for (int i = 0; i < result; ++i) {
        auto item_id = ++object_id;
        auto result = EM_ASM_INT({
          const result = dromozoa_web_dom.get($1).item($2);
          dromozoa_web_dom.set($0, result);
          return result.nodeType;
        }, item_id, id, i);
        new_userdata<object_t>(L, node_type_to_name(result), item_id);
        lua_seti(L, -2, i + 1);
      }

      EM_ASM({
        dromozoa_web_dom.delete($0);
      }, id);
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
        initialize_interface(L, "dromozoa.web.dom.document");
        set_metafield(L, -1, "__call", function<impl_document_call>());
        initialize_parent_node(L);
      }
      lua_setfield(L, -2, "document");

      lua_newtable(L);
      {
        initialize_interface(L, "dromozoa.web.dom.element");
        set_field(L, -1, "get_attribute_names", function<impl_get_attribute_names>());
        set_field(L, -1, "get_attribute", function<impl_get_attribute>());
        set_field(L, -1, "set_attribute", function<impl_set_attribute>());
        set_field(L, -1, "remove_attribute", function<impl_remove_attribute>());
        initialize_parent_node(L);
      }
      lua_setfield(L, -2, "element");
    }
  }
}
