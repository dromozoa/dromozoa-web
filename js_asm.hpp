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

#ifndef DROMOZOA_WEB_JS_ASM_HPP
#define DROMOZOA_WEB_JS_ASM_HPP

#include <emscripten.h>
#include <cstdlib>
#include <memory>
#include "error.hpp"

namespace dromozoa {
  inline void js_asm_impl(const char* file, int line, void* result) {
    if (std::unique_ptr<char, decltype(&std::free)> error{static_cast<char*>(result), std::free}) {
      throw std::logic_error(make_error(file, line, error.get()));
    }
  }
}

#define DROMOZOA_JS_ASM(code, ...) \
  dromozoa::js_asm_impl(__FILE__, __LINE__, emscripten_asm_const_ptr(CODE_EXPR( \
    "try {" \
    #code \
    ";} catch (e) { return allocateUTF8(e.toString()); } return 0;" \
  ) _EM_ASM_PREP_ARGS(__VA_ARGS__))) \
/**/

#endif
