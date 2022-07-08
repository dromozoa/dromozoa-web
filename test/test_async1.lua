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
local await = async.await

local window = D.window

local v = 0
async.delay(function ()
  print(v)
  assert(v == 1)
  v = 2
end)
print(v)
assert(v == 0)
v = 1

local future = async(function (self)
  print("delay", D.get_now())
  await(function (self)
    self:resume(true)
  end)
  print("delay", D.get_now())

  print("setTimeout", D.get_now())
  await(function (self)
    window:setTimeout(function ()
      self:resume(true)
    end, 1000)
  end)
  print("setTimeout", D.get_now())

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
