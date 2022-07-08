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

print("device_pixel_ratio", D.get_device_pixel_ratio())
print("window_title", D.get_window_title())
D.set_window_title "タイトル変更"
print("window_title", D.get_window_title())
print("screen_size", D.get_screen_size())

local data = {}
local n = 100

print "FPSを計測中"

for i = 1, n + 1 do
  data[i] = D.get_now()
  coroutine.yield()
end

local sum = (data[n + 1] - data[1])
local avg = sum / n

local v = 0
for i = 1, n do
  v = v + (data[i + 1] - data[i])^2
end

print("sum", sum)
print("avg", avg)
print("sd", math.sqrt(v))
print("fps", 1000 / avg)
