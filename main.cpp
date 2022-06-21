#include <emscripten.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <iostream>
#include <stdexcept>

class context_t {
public:
  context_t() : state_() {
    state_ = luaL_newstate();
    luaL_openlibs(state_);
  }

  ~context_t() {
    lua_close(state_);
    state_ = nullptr;
  }

  void load();
  void each();

private:
  lua_State* state_;
};

void context_t::load() {
  lua_State* L = state_;

  static const char code[] =
  #include "main.lua"
  ;

  if (luaL_loadbuffer(L, code, strlen(code), "=(load)") != 0) {
    std::cerr << "could not luaL_loadbuffer: " << lua_tostring(L, -1) << "\n";
    // error
  }
  lua_pcall(L, 0, 1, 0);
}

void context_t::each() {
  lua_State* L = state_;
  if (lua_type(L, -1) == LUA_TFUNCTION) {
    lua_pushvalue(L, -1);
    lua_pcall(L, 0, 0, 0);
  }
}

void each(void* data) {
  context_t* context = static_cast<context_t*>(data);
  context->each();
}

int main() {
  context_t* context = new context_t();
  context->load();
  emscripten_set_main_loop_arg(each, context, 30, true);
  return 0;
}
