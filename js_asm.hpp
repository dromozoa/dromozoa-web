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
  template <class T>
  inline std::unique_ptr<T, decltype(&std::free)> make_unique_impl(void* ptr) {
    return std::unique_ptr<T, decltype(&std::free)>(static_cast<T*>(ptr), std::free);
  }
}

#define DROMOZOA_JS_ASM_PROLOGUE \
  "try {" \
/**/

#define DROMOZOA_JS_ASM_EPILOGUE \
  "} catch (e) {" \
    "const size = lengthBytesUTF8(e.message) + 1;" \
    "const data = _malloc(size);" \
    "stringToUTF8(e.message, data, size);" \
    "return data;" \
  "}" \
  "return 0;" \
/**/

#define DROMOZOA_JS_ASM(code, ...) \
  if (auto error = dromozoa::make_unique_impl<char>(emscripten_asm_const_ptr(CODE_EXPR(DROMOZOA_JS_ASM_PROLOGUE #code DROMOZOA_JS_ASM_EPILOGUE) _EM_ASM_PREP_ARGS(__VA_ARGS__)))) \
    throw DROMOZOA_LOGIC_ERROR("javascript error: ", error.get()) \
/**/

#endif
