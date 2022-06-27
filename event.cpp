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
    void set_event(lua_State* L, const EmscriptenMouseEvent* event) {
      set_field(L, -1, "timestamp", event->timestamp);
      set_field(L, -1, "screen_x", event->screenX);
      set_field(L, -1, "screen_y", event->screenY);
      set_field(L, -1, "client_x", event->clientX);
      set_field(L, -1, "client_y", event->clientY);
      set_field(L, -1, "ctrl_key", static_cast<bool>(event->ctrlKey));
      set_field(L, -1, "shift_key", static_cast<bool>(event->shiftKey));
      set_field(L, -1, "alt_key", static_cast<bool>(event->altKey));
      set_field(L, -1, "meta_key", static_cast<bool>(event->metaKey));
      set_field(L, -1, "button", event->button);
      set_field(L, -1, "buttons", event->buttons);
      set_field(L, -1, "movement_x", event->movementX);
      set_field(L, -1, "movement_y", event->movementY);
      set_field(L, -1, "target_x", event->targetX);
      set_field(L, -1, "target_y", event->targetY);
    }

    void set_event(lua_State* L, const EmscriptenWheelEvent* event) {
      set_event(L, &event->mouse);
      set_field(L, -1, "delta_x", event->deltaX);
      set_field(L, -1, "delta_y", event->deltaY);
      set_field(L, -1, "delta_z", event->deltaZ);
      set_field(L, -1, "delta_mode", event->deltaMode);
    }

    class callback_base_t {
    public:
      virtual ~callback_base_t() {}
    };

    template <class T>
    class callback_t : public callback_base_t {
    public:
      explicit callback_t(thread_reference&& ref) : ref_(std::move(ref)) {}

      static EM_BOOL callback(int event_type, const T* event, void* data) {
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
          lua_pushvalue(L, 1);
          push(L, event_type);
          lua_newtable(L);
          set_event(L, event);
          if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
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

    void push_result(lua_State* L, int result) {
      switch (result) {
      case EMSCRIPTEN_RESULT_SUCCESS:
      case EMSCRIPTEN_RESULT_DEFERRED:
        push(L, true);
        break;
      default:
        lua_pushnil(L);
      }
      push(L, result);
    }

    template <class T>
    void set_callback(
        lua_State* L,
        int event_type,
        EMSCRIPTEN_RESULT (*set_on_thread)(const char*, void*, EM_BOOL, EM_BOOL (*)(int, const T*, void*), pthread_t),
        pthread_t thread = EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD) {
      const auto* target = luaL_checkstring(L, 1);
      auto use_capture = lua_toboolean(L, 2);
      bool deregister = lua_isnoneornil(L, 3);

      callback_t<T>* self = nullptr;

      lua_getfield(L, LUA_REGISTRYINDEX, "dromozoa.web.event.callbacks");
      push(L, make_callback_key(event_type, target));
      if (deregister) {
        lua_pushnil(L);
      } else {
        thread_reference ref(L);
        lua_pushvalue(L, 3);
        lua_xmove(L, ref.get(), 1);
        self = new_userdata<callback_t<T> >(L, "dromozoa.web.event.callback", std::move(ref));
      }
      lua_settable(L, -3);
      lua_pop(L, 1);

      push_result(L, set_on_thread(target, self, use_capture, self ? callback_t<T>::callback : nullptr, thread));
    }

    void impl_gc(lua_State* L) {
      static_cast<callback_base_t*>(luaL_checkudata(L, 1, "dromozoa.web.event.callback"))->~callback_base_t();
    }

    void impl_set_click_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_CLICK, emscripten_set_click_callback_on_thread);
    }

    void impl_set_mousedown_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEDOWN, emscripten_set_mousedown_callback_on_thread);
    }

    void impl_set_mouseup_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEUP, emscripten_set_mouseup_callback_on_thread);
    }

    void impl_set_dblclick_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_DBLCLICK, emscripten_set_dblclick_callback_on_thread);
    }

    void impl_set_mousemove_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEMOVE, emscripten_set_mousemove_callback_on_thread);
    }

    void impl_set_mouseenter_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEENTER, emscripten_set_mouseenter_callback_on_thread);
    }

    void impl_set_mouseleave_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSELEAVE, emscripten_set_mouseleave_callback_on_thread);
    }

    void impl_set_mouseover_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEOVER, emscripten_set_mouseover_callback_on_thread);
    }

    void impl_set_mouseout_callback(lua_State* L) {
      set_callback<EmscriptenMouseEvent>(L, EMSCRIPTEN_EVENT_MOUSEOUT, emscripten_set_mouseout_callback_on_thread);
    }

    void impl_set_wheel_callback(lua_State* L) {
      set_callback<EmscriptenWheelEvent>(L, EMSCRIPTEN_EVENT_WHEEL, emscripten_set_wheel_callback_on_thread);
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
      set_field(L, -1, "set_mousedown_callback", function<impl_set_mousedown_callback>());
      set_field(L, -1, "set_mouseup_callback", function<impl_set_mouseup_callback>());
      set_field(L, -1, "set_dblclick_callback", function<impl_set_dblclick_callback>());
      set_field(L, -1, "set_mousemove_callback", function<impl_set_mousemove_callback>());
      set_field(L, -1, "set_mouseenter_callback", function<impl_set_mouseenter_callback>());
      set_field(L, -1, "set_mouseleave_callback", function<impl_set_mouseleave_callback>());
      set_field(L, -1, "set_mouseover_callback", function<impl_set_mouseover_callback>());
      set_field(L, -1, "set_mouseout_callback", function<impl_set_mouseout_callback>());
      set_field(L, -1, "set_mouseout_callback", function<impl_set_mouseout_callback>());
      set_field(L, -1, "set_wheel_callback", function<impl_set_wheel_callback>());
    }
  }
}
