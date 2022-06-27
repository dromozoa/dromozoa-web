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

#include <emscripten/html5.h>
#include <iostream>
#include <sstream>
#include <string>
#include "common.hpp"
#include "error.hpp"
#include "exception_queue.hpp"
#include "lua.hpp"
#include "thread_reference.hpp"

namespace dromozoa {
  namespace {
    class callback_base_t {
    public:
      virtual ~callback_base_t() {
        std::cout << "~callback_base_t()\n";
      }
    };

    template <class T>
    class callback_t : public callback_base_t {
    public:
      explicit callback_t(thread_reference&& ref) : ref_(std::move(ref)) {}

      static EM_BOOL callback(int event_type, const T* event, void* data) {
        std::cout << "callback " << event_type << " " << event << " " << data << "\n";

        if (auto* self = static_cast<callback_t<T>*>(data)) {
          return self->callback_impl(event_type, event);
        }
        return false;
      }

    private:
      thread_reference ref_;

      bool callback_impl(int event_type, const T* event) {
        try {
          lua_State* L = ref_.get();
          std::cout << "pcall " << L << "\n";
          lua_pushvalue(L, 1);
          if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            throw DROMOZOA_LOGIC_ERROR("canot lua_pcall: ", lua_tostring(L, -1));
          }
          if (lua_toboolean(L, -1) || !lua_isboolean(L, -1)) {
            return true;
          }
        } catch (...) {
          push_exception_queue();
        }
        return false;
      }
    };

    std::string make_callback_key(int event_type, const char* target) {
      std::ostringstream out;
      out << event_type << ".";
      if (target == EMSCRIPTEN_EVENT_TARGET_DOCUMENT) {
        out << "EMSCRIPTEN_EVENT_TARGET_DOCUMENT";
      } else if (target == EMSCRIPTEN_EVENT_TARGET_WINDOW) {
        out << "EMSCRIPTEN_EVENT_TARGET_WINDOW";
      } else if (target == EMSCRIPTEN_EVENT_TARGET_SCREEN) {
        out << "EMSCRIPTEN_EVENT_TARGET_SCREEN";
      } else {
        out << target;
      }
      return out.str();
    }

    void impl_gc(lua_State* L) {
      static_cast<callback_base_t*>(luaL_checkudata(L, 1, "dromozoa.web.event.callback"))->~callback_base_t();
    }

    void impl_set_click_callback(lua_State* L) {
      const auto* target = luaL_checkstring(L, 1);
      auto use_capture = lua_toboolean(L, 2);
      bool is_noneornil = lua_isnoneornil(L, 3);

      callback_t<EmscriptenMouseEvent>* self = nullptr;

      lua_getfield(L, LUA_REGISTRYINDEX, "dromozoa.web.event.callbacks");
      std::cout << "top " << lua_gettop(L) << "\n";
      push(L, make_callback_key(EMSCRIPTEN_EVENT_CLICK, target));
      if (is_noneornil) {
        std::cout << "is none or nil\n";
        lua_pushnil(L);
      } else {
        std::cout << "is not nil\n";
        thread_reference ref(L);
        lua_pushvalue(L, 3);
        lua_xmove(L, ref.get(), 1);
        self = new_userdata<callback_t<EmscriptenMouseEvent> >(L, "dromozoa.web.event.callback", std::move(ref));
      }
      lua_settable(L, -3);

      std::cout << "impl_set_click_callback " << target << " " << self << "\n";
      if (self) {
        emscripten_set_click_callback(target, self, use_capture, callback_t<EmscriptenMouseEvent>::callback);
      } else {
        emscripten_set_click_callback(target, self, use_capture, nullptr);
      }
    }
  }

  void initialize_event(lua_State* L) {
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, "dromozoa.web.event.callbacks");

    lua_newtable(L);
    {
      luaL_newmetatable(L, "dromozoa.web.event.callback");
      set_field(L, -1, "__gc", function<impl_gc>());
      lua_pop(L, 1);

      set_field(L, -1, "set_click_callback", function<impl_set_click_callback>());
    }
  }
}
