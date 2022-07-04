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

#ifndef DROMOZOA_WEB_JS_ERROR_HPP
#define DROMOZOA_WEB_JS_ERROR_HPP

#include <stdexcept>
#include "lua.hpp"

namespace dromozoa {
  class js_error : public std::logic_error {
  public:
    static constexpr char NAME[] = "dromozoa.web.js_error";
    explicit js_error(const char* what) : std::logic_error(what) {}
  };

  void initialize_js_error(lua_State*);
}

#endif
