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
  get_thread:    cwrap("dromozoa_web_get_thread",  "pointer", [                   ]),
  load_string:   cwrap("dromozoa_web_load_string", "number",  ["pointer", "string"]),
  call:          cwrap("dromozoa_web_call",        "number",  ["pointer", "number"]),
  push_nil:      cwrap("dromozoa_web_push_nil",    null,      ["pointer"          ]),
  push_number:   cwrap("dromozoa_web_push_number", null,      ["pointer", "number"]),
  push_boolean:  cwrap("dromozoa_web_push_boolean",null,      ["pointer", "number"]),
  push_string:   cwrap("dromozoa_web_push_string", null,      ["pointer", "string"]),
  push_symbol:   cwrap("dromozoa_web_push_symbol", null,      ["pointer", "number"]),
  push_null:     cwrap("dromozoa_web_push_null",   null,      ["pointer"          ]),
  push_object:   cwrap("dromozoa_web_push_object", null,      ["pointer", "number"]),
  push_ref:      cwrap("dromozoa_web_push_ref",    null,      ["number"           ]),
  ref:           cwrap("dromozoa_web_ref",         "number",  ["pointer"          ]),
  unref:         cwrap("dromozoa_web_unref",       null,      ["pointer", "number"]),

  do_string: (code) => {
    const L = D.get_thread();
    if (L) {
      if (D.load_string(L, code)) {
        switch (D.call(L, 0)) {
          case 1:
            return D.stack.pop();
          case 2:
            throw new Error(D.stack.pop());
        }
      }
    }
  },

  push: (L, v) => {
    switch (typeof v) {
      case "undefined":
        D.push_nil(L, v);
        break;
      case "number":
        D.push_number(L, v);
        break;
      case "boolean":
        D.push_boolean(L, !!v);
        break;
      case "string":
        D.push_string(L, v);
        break;
      case "symbol":
        D.push_symbol(L, D.ref_object(v));
        break;
      default:
        if (v === null) {
          D.push_null(L);
        } else {
          D.push_object(L, D.ref_object(v));
        }
    }
  },

  ref_object: (obj) => {
    let ref = D.objs[0];
    if (ref === 0) {
      ref = D.objs.length;
    } else {
      D.objs[0] = D.objs[ref]
    }
    D.objs[ref] = obj;
    return ref;
  },

  unref_object: (ref) => {
    if (ref !== 0) {
      D.objs[ref] = D.objs[0];
      D.objs[0] = ref;
    }
  },

  stack: [],
  objs: [ 0 ],
  refs: new FinalizationRegistry((ref) => {
    const L = D.get_thread();
    if (L) {
      D.unref(L, ref);
    }
  }),
};
