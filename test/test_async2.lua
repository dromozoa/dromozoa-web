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
local navigator = window.navigator

local future = async(function (self)
  local devices = await(navigator.mediaDevices:enumerateDevices())
  devices:forEach(function (device)
    io.write(("kind=%s, label=%s, deviceId=%s\n"):format(device.kind, device.label, device.deviceId))
  end)

  local result = await(function (self)
    local result = window:prompt()
    if result == D.null then
      self:resume(true)
    else
      self:resume(true, result)
    end
  end)
  if result then
    print("ok", result)
  else
    print "cancel"
  end

  print "finished"
end)

-- await(function (self) self:resume(true) end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
