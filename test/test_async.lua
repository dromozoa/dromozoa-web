-- Copyright (C) 2022 Tomoyuki Fujimori <moyu@dromozoa.com>
--
-- This file is part of dromozoa-web.
--
-- dromozoa-web is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- dromozoa-web is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with dromozoa-web.  If not, see <http://www.gnu.org/licenses/>.

local D = require "dromozoa.web"

local thread = coroutine.create(function (thread)
  D.window:setTimeout(function ()
    print("C1", thread, coroutine.status(thread))
    print("C2", coroutine.resume(thread, "C2"))
    print("C3", thread, coroutine.status(thread))
  end, 200)

  print("B1", thread, coroutine.status(thread))
  print("B2", coroutine.yield())
  print("B3", thread, coroutine.status(thread))

  D.window:setTimeout(function ()
    print("D1", thread, coroutine.status(thread))
    print("D2", coroutine.resume(thread, "D2"))
    print("D3", thread, coroutine.status(thread))
  end, 200)

  print("E1", thread, coroutine.status(thread))
  print("E2", coroutine.yield())
  print("E3", thread, coroutine.status(thread))

  error "XXX"

  D.window:setTimeout(function ()
    print("F1", thread, coroutine.status(thread))
    print("F2", coroutine.resume(thread, "F2"))
    print("F3", thread, coroutine.status(thread))
  end, 200)

  error "YYY"

  print("G1", thread, coroutine.status(thread))
  print("G2", coroutine.yield())
  print("G3", thread, coroutine.status(thread))

  return "Z"
end)

print("A1", thread, coroutine.status(thread))
print("A2", coroutine.resume(thread, thread))
print("A3", thread, coroutine.status(thread))

while true do
  assert(D.get_error_queue())
  coroutine.yield()
end
