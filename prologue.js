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

const D = {
  id: 0,
  objects: new Map(),
  stack: [],
  generate_id: () => { return ++D.id; },

  refs: new FinalizationRegistry((v) => { D.unref(D.get_state(), v); }),

  evaluate_lua: cwrap("dromozoa_web_evaluate_lua", null, ["string"]),
  get_state: cwrap("dromozoa_web_get_state", "pointer", []),
  push_function: cwrap("dromozoa_web_push_function", null, ["number"]),
  call_function: cwrap("dromozoa_web_call_function", null, ["pointer", "number"]),
  push_nil: cwrap("dromozoa_web_push_nil", null, ["pointer"]),
  push_null: cwrap("dromozoa_web_push_null", null, ["pointer"]),
  push_integer: cwrap("dromozoa_web_push_integer", null, ["pointer", "number"]),
  push_number: cwrap("dromozoa_web_push_number", null, ["pointer", "number"]),
  push_boolean: cwrap("dromozoa_web_push_boolean", null, ["pointer", "number"]),
  push_string: cwrap("dromozoa_web_push_string", null, ["pointer", "string"]),
  push_object: cwrap("dromozoa_web_push_object", null, ["pointer", "number"]),
  unref: cwrap("dromozoa_web_unref", null, ["pointer", "number"]),

  push: (L, v) => {
    switch (typeof v) {
      case "undefined":
        D.push_nil(L, v);
        break;
      case "number":
        if (Number.isInteger(v)) {
          D.push_integer(L, v);
        } else {
          D.push_number(L, v);
        }
        break;
      case "boolean":
        D.push_boolean(L, !!v);
        break;
      case "string":
        D.push_string(L, v);
        break;
      default:
        if (v === null) {
          D.push_null(L);
        } else {
          const id = D.generate_id();
          D.objects.set(id, v);
          D.push_object(L, id);
        }
    }
  },

  new: (T, ...a) => {
    return new T(a);
  },
};
