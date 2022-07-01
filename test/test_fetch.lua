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

local prototype = D.window.Promise.prototype
prototype.then_ = prototype["then"]

local p1 = D.window:fetch("main.lua", {
  headers = {
    ["X-Test-Header1"] = "foo";
    ["X-Test-Header2"] = "bar";
  };
  cache = "no-store";
})

local done

local p2 = p1:then_(function (response)
  local p = response:text()
  return p
end)
local p3 = p2:then_(function (text)
  print(text)
  done = true
end)
local p4 = p3:catch(function (e)
  print("catch", e.message)
  done = true
end)

while not done do
  coroutine.yield()
end

D.window.console:log { foo = 17, bar = 23, baz = D.array { 1, 2, D.null, 3, qux = true } }

local array = D.array { 1, 2, 3, qux = true }
print(array)
print(#array, array[1], array.qux)
print(D.window.JSON:stringify({
  [{}] = "table1";
  [{}] = "table2";
  foo = 17,
  bar = 23,
  baz = D.array { 1, 2, D.null, 3, qux = true }
}))
