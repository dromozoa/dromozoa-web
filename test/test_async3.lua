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

local D, G = require "dromozoa.web" :import "global"
local async, await = require "dromozoa.web.async" :import "await"

local f1 = async(function ()
  print "f1 started"
  for i = 1, 10 do
    print("f1", i)
    await(function (promise) G:setTimeout(function () promise:set(true) end, 200) end)
  end
  print "f1 finished"
  return "f1"
end)

local f2 = async(function ()
  print "f2 started"
  for i = 1, 9 do
    print("f2", i)
    await(function (promise) G:setTimeout(function () promise:set(true) end, 200) end)
  end
  print "f2 finished"
  return "f2"
end)

local future = async(function ()
  print("AAA", f1)
  local r1 = await(f1)
  print("BBB", f2)
  local r2 = await(f2)
  print "CCC"
  return r1 .. r2
end)

while true do
  if future and future:is_ready() then
    print(future:get())
    future = nil
  end
  coroutine.yield()
end
