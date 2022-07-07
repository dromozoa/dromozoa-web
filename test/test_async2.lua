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
local async = require "dromozoa.web.async"

-- error
-- error "!!!"

local function timeout_promise(t, k)
  return D.new(D.window.Promise, function (resolve, reject)
    D.window:setTimeout(function ()
      resolve(nil, k)
    end, t)
  end)
end

local f = async(function (self)
  print "start"
  local p = timeout_promise(500, "A")
  print(D.typeof(p), D.typeof(42), D.typeof(function () end))
  print(D.instanceof(p, D.window.Object), D.instanceof(p, D.window.Promise), D.instanceof(p, D.window.Array))
  print(D.instanceof(42, D.window.Object), D.instanceof(42, D.window.Promise), D.instanceof(42, D.window.Array))
  print(self:await(p))
  print(self:await(timeout_promise(500, "B")))
  print(self:await(timeout_promise(500, "C")))
  -- error "X"
  print(self:await(timeout_promise(500, "D")))
  print "goal"
  return "z"
end)

while true do
  assert(D.get_error_queue())
  if f and f:is_ready() then
    local v = f:get()
    f = nil
    print(v)
  end
  coroutine.yield()
end
