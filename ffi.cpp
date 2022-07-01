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
#include <cstring>
#include <deque>
#include <exception>
#include <memory>
#include "common.hpp"
#include "error.hpp"
#include "lua.hpp"
#include "noncopyable.hpp"
#include "stack_guard.hpp"

#define DROMOZOA_JS_ASM_TRY \
  "try {" \
/**/

#define DROMOZOA_JS_ASM_CATCH \
  "} catch (e) {" \
    "const size = lengthBytesUTF8(e.message) + 1;" \
    "const data = _malloc(size);" \
    "stringToUTF8(e.message, data, size);" \
    "return data;" \
  "}" \
  "return 0;" \
/**/

#define DROMOZOA_JS_ASM(code, ...) \
  if (auto cstr = dromozoa::make_unique_cstr(emscripten_asm_const_ptr(CODE_EXPR(DROMOZOA_JS_ASM_TRY #code DROMOZOA_JS_ASM_CATCH) _EM_ASM_PREP_ARGS(__VA_ARGS__)))) \
    throw DROMOZOA_LOGIC_ERROR("javascript error: ", cstr.get()) \
/**/

namespace dromozoa {
  namespace {
    std::unique_ptr<char, decltype(&std::free)> make_unique_cstr(void* ptr) {
      return std::unique_ptr<char, decltype(&std::free)>(static_cast<char*>(ptr), std::free);
    }

    lua_State* thread = nullptr;

    std::deque<std::exception_ptr> error_queue;

    void push_error() {
      error_queue.emplace_back(std::current_exception());
    }

    constexpr char NAME_ERROR[] = "dromozoa.web.error";

    class error_t : noncopyable {
    public:
      explicit error_t(const std::string& what) : what_(what) {}

      const char* get() const {
        return what_.c_str();
      }

    private:
      std::string what_;
    };

    error_t* test_error(lua_State* L, int index) {
      return static_cast<error_t*>(luaL_testudata(L, index, NAME_ERROR));
    }

    error_t* check_error(lua_State* L, int index) {
      return static_cast<error_t*>(luaL_checkudata(L, index, NAME_ERROR));
    }

    void impl_gc_error(lua_State* L) {
      check_error(L, 1)->~error_t();
    }

    constexpr char NAME_ARRAY[] = "dromozoa.web.array";

    bool is_array(lua_State* L, int index) {
      stack_guard guard(L);
      if (lua_getmetatable(L, index)) {
        luaL_getmetatable(L, NAME_ARRAY);
        if (lua_rawequal(L, -1, -2)) {
          return true;
        }
      }
      return false;
    }

    constexpr char NAME_OBJECT[] = "dromozoa.web.object";

    class object_t : noncopyable {
    public:
      explicit object_t(int ref) : ref_(ref) {}

      ~object_t() {
        close();
      }

      int get() const {
        return ref_;
      };

      void close() {
        if (ref_) {
          DROMOZOA_JS_ASM({ D.unref_object($0); }, ref_);
          ref_ = 0;
        }
      }

    private:
      int ref_;
    };

    object_t* test_object(lua_State* L, int index) {
      return static_cast<object_t*>(luaL_testudata(L, index, NAME_OBJECT));
    }

    object_t* check_object(lua_State* L, int index) {
      return static_cast<object_t*>(luaL_checkudata(L, index, NAME_OBJECT));
    }

    void js_push(lua_State* L, int index) {
      switch (lua_type(L, index)) {
        case LUA_TNONE:
        case LUA_TNIL:
          DROMOZOA_JS_ASM({ D.stack.push(undefined); });
          break;
        case LUA_TNUMBER:
          DROMOZOA_JS_ASM({ D.stack.push($0); }, lua_tonumber(L, index));
          break;
        case LUA_TBOOLEAN:
          DROMOZOA_JS_ASM({ D.stack.push(!!$0); }, lua_toboolean(L, index));
          break;
        case LUA_TSTRING:
          DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, lua_tostring(L, index));
          break;
        case LUA_TTABLE:
          {
            index = lua_absindex(L, index);

            auto array = is_array(L, index);
            if (array) {
              DROMOZOA_JS_ASM({ D.stack.push([]); });
            } else {
              DROMOZOA_JS_ASM({ D.stack.push({}); });
            }

            lua_pushnil(L);
            while (lua_next(L, index)) {
              switch (lua_type(L, -2)) {
                case LUA_TNUMBER:
                  if (array && lua_isinteger(L, -2)) {
                    DROMOZOA_JS_ASM({ D.stack.push($0); }, lua_tonumber(L, -2) - 1);
                  } else {
                    DROMOZOA_JS_ASM({ D.stack.push($0); }, lua_tonumber(L, -2));
                  }
                  break;
                case LUA_TSTRING:
                  DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, lua_tostring(L, -2));
                  break;
                default:
                  lua_pop(L, 1);
                  continue;
              }
              js_push(L, -1);
              DROMOZOA_JS_ASM({
                const v = D.stack.pop();
                const k = D.stack.pop();
                D.stack[D.stack.length - 1][k] = v;
              });
              lua_pop(L, 1);
            }
          }
          break;
        case LUA_TFUNCTION:
          {
            DROMOZOA_JS_ASM({
              const v = (...args) => {
                const L = D.get_thread();
                if (L) {
                  const n = args.length;
                  D.push_ref(L, v.ref);
                  for (let i = 0; i < n; ++i) {
                    D.push(L, args[i]);
                  }
                  switch (D.call(L, n)) {
                    case 1:
                      return D.stack.pop();
                    case 2:
                      throw new Error(D.stack.pop());
                  }
                }
              };
              v.ref = D.ref_registry($0, $1);
              D.refs.register(v, v.ref);
              D.stack.push(v);
            }, L, index);
          }
          break;
        case LUA_TUSERDATA:
          if (auto* that = test_object(L, index)) {
            DROMOZOA_JS_ASM({ D.stack.push(D.objs[$0]); }, that->get());
          } else {
            throw DROMOZOA_LOGIC_ERROR(NAME_OBJECT, " expected, got ", luaL_typename(L, index));
          }
          break;
        case LUA_TLIGHTUSERDATA:
          if (!lua_touserdata(L, index)) {
            DROMOZOA_JS_ASM({ D.stack.push(null); });
          } else {
            throw DROMOZOA_LOGIC_ERROR("null lightuserdata expected, got non-null lightuserdata");
          }
          break;
        default:
          throw DROMOZOA_LOGIC_ERROR("unexpected ", luaL_typename(L, index));
      }
    }

    void impl_eq(lua_State* L) {
      auto* self = test_object(L, 1);
      auto* that = test_object(L, 1);
      if (self && that) {
        DROMOZOA_JS_ASM({ D.push_boolean($0, D.objs[$1] === D.objs[$2]); }, L, self->get(), that->get());
      } else {
        lua_pushboolean(L, false);
      }
    }

    void impl_index(lua_State* L) {
      auto* self = check_object(L, 1);
      switch (lua_type(L, 2)) {
        case LUA_TNUMBER:
          DROMOZOA_JS_ASM({ D.push($0, D.objs[$1][$2]); }, L, self->get(), lua_tonumber(L, 2));
          break;
        case LUA_TSTRING:
          DROMOZOA_JS_ASM({ D.push($0, D.objs[$1][UTF8ToString($2)]); }, L, self->get(), lua_tostring(L, 2));
          break;
        default:
          luaL_typeerror(L, 2, "number or string");
      }
    }

    void impl_newindex(lua_State* L) {
      auto* self = check_object(L, 1);
      switch (lua_type(L, 2)) {
        case LUA_TNUMBER:
          js_push(L, 3);
          DROMOZOA_JS_ASM({ D.objs[$0][$1] = D.stack.pop(); }, self->get(), lua_tonumber(L, 2));
          break;
        case LUA_TSTRING:
          js_push(L, 3);
          DROMOZOA_JS_ASM({ D.objs[$0][UTF8ToString($1)] = D.stack.pop(); }, self->get(), lua_tostring(L, 2));
          break;
        default:
          luaL_typeerror(L, 2, "number or string");
      }
    }

    void impl_call(lua_State* L) {
      auto* self = check_object(L, 1);
      auto top = lua_gettop(L);

      js_push(L, 2);
      DROMOZOA_JS_ASM({
        D.args = [];
        D.args.thisArg = D.stack.pop();
      });

      for (auto i = 3; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM({ D.args.push(D.stack.pop()); });
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.args = undefined;
        D.push($0, D.objs[$1].apply(args.thisArg, args));
      }, L, self->get());
    }

    void impl_close_object(lua_State* L) {
      check_object(L, 1)->close();
    }

    void impl_gc_object(lua_State* L) {
      check_object(L, 1)->~object_t();
    }

    void impl_gc_module(lua_State*) {
      thread = nullptr;
    }

    void impl_get_error(lua_State* L) {
      try {
        if (!error_queue.empty()) {
          auto e = error_queue.front();
          error_queue.pop_front();
          std::rethrow_exception(e);
        }
        lua_pushnil(L);
      } catch (const std::exception& e) {
        lua_pushstring(L, e.what());
      } catch (...) {
        lua_pushstring(L, "unknown error");
      }
    }

    void impl_new(lua_State* L) {
      auto top = lua_gettop(L);

      DROMOZOA_JS_ASM({ D.args = []; });

      for (auto i = 1; i <= top; ++i) {
        js_push(L, i);
        DROMOZOA_JS_ASM({ D.args.push(D.stack.pop()); });
      }

      DROMOZOA_JS_ASM({
        const args = D.args;
        D.args = undefined;
        D.push($0, D.new.apply(undefined, args));
      }, L);
    }

    void impl_ref(lua_State* L) {
      js_push(L, 1);
      DROMOZOA_JS_ASM({ D.push_object($0, D.ref_object(D.stack.pop())); }, L);
    }

    void impl_throw(lua_State* L) {
      const char* what = luaL_checkstring(L, 1);
      new_userdata<error_t>(L, NAME_ERROR, what);
      lua_error(L);
    }

    void impl_array(lua_State* L) {
      luaL_setmetatable(L, NAME_ARRAY);
    }
  }

  void initialize_ffi(lua_State* L) {
    thread = lua_newthread(L);
    luaL_ref(L, LUA_REGISTRYINDEX);

    luaL_newmetatable(L, NAME_ERROR);
    set_field(L, -1, "__gc", function<impl_gc_error>());
    lua_pop(L, 1);

    luaL_newmetatable(L, NAME_ARRAY);
    lua_pop(L, 1);

    luaL_newmetatable(L, NAME_OBJECT);
    set_field(L, -1, "__eq", function<impl_eq>());
    set_field(L, -1, "__index", function<impl_index>());
    set_field(L, -1, "__newindex", function<impl_newindex>());
    set_field(L, -1, "__call", function<impl_call>());
    set_field(L, -1, "__close", function<impl_close_object>());
    set_field(L, -1, "__gc", function<impl_gc_object>());
    lua_pop(L, 1);

    set_metafield(L, -1, "__gc", function<impl_gc_module>());

    set_field(L, -1, "get_error", function<impl_get_error>());
    set_field(L, -1, "new", function<impl_new>());
    set_field(L, -1, "ref", function<impl_ref>());
    set_field(L, -1, "throw", function<impl_throw>());
    set_field(L, -1, "array", function<impl_array>());

    DROMOZOA_JS_ASM({ D.push_object($0, D.ref_object(window)); }, L);
    lua_setfield(L, -2, "window");

    lua_pushlightuserdata(L, nullptr);
    lua_setfield(L, -2, "null");
  }
}

extern "C" {
  using namespace dromozoa;

  lua_State* EMSCRIPTEN_KEEPALIVE dromozoa_web_get_thread() {
    return thread;
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_evaluate(lua_State* L, const char* code) {
    try {
      stack_guard guard(L);
      if (luaL_loadbuffer(L, code, std::strlen(code), "=(load)") != LUA_OK) {
        throw DROMOZOA_LOGIC_ERROR("cannot luaL_loadbuffer: ", lua_tostring(L, -1));
      }
      if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        if (auto* that = test_error(L, -1)) {
          DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, that->get());
          return 2;
        } else {
          throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
        }
      } else {
        js_push(L, -1);
        return 1;
      }
    } catch (...) {
      push_error();
    }
    return 0;
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_call(lua_State* L, int n) {
    try {
      stack_guard guard(L);
      if (lua_pcall(L, n, 1, 0) != LUA_OK) {
        if (auto* that = test_error(L, -1)) {
          DROMOZOA_JS_ASM({ D.stack.push(UTF8ToString($0)); }, that->get());
          return 2;
        } else {
          throw DROMOZOA_LOGIC_ERROR("cannot lua_pcall: ", lua_tostring(L, -1));
        }
      } else {
        js_push(L, -1);
        return 1;
      }
    } catch (...) {
      push_error();
    }
    return 0;
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_nil(lua_State* L) {
    lua_pushnil(L);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_integer(lua_State* L, int value) {
    lua_pushinteger(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_number(lua_State* L, double value) {
    lua_pushnumber(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_boolean(lua_State* L, int value) {
    lua_pushboolean(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_string(lua_State* L, const char* value) {
    lua_pushstring(L, value);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_null(lua_State* L) {
    lua_pushlightuserdata(L, nullptr);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_object(lua_State* L, int id) {
    new_userdata<object_t>(L, NAME_OBJECT, id);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_push_ref(lua_State* L, int ref) {
    lua_geti(L, LUA_REGISTRYINDEX, ref);
  }

  int EMSCRIPTEN_KEEPALIVE dromozoa_web_ref_registry(lua_State* L, int index) {
    lua_pushvalue(L, index);
    return luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void EMSCRIPTEN_KEEPALIVE dromozoa_web_unref_registry(lua_State* L, int ref) {
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
  }
}
