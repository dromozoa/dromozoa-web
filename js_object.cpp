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

#include "js_asm.hpp"
#include "common.hpp"
#include "js_object.hpp"
#include "udata.hpp"

namespace dromozoa {
  namespace {
    void impl_eq(lua_State* L) {
      auto* self = test_udata<js_object>(L, 1);
      auto* that = test_udata<js_object>(L, 2);
      if (self && that) {
        DROMOZOA_JS_ASM({ D.push_boolean($0, D.objs[$1] === D.objs[$2]); }, L, self->get(), that->get());
      } else {
        push(L, false);
      }
    }
  }

  void js_object::close() noexcept {
    if (ref_) {
      try {
        DROMOZOA_JS_ASM({ D.unref_object($0) }, ref_);
      } catch (...) {}
      ref_ = 0;
    }
  }

  void initialize_js_object(lua_State* L) {
  }
}
