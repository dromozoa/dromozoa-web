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

local window = D.window
local document = window.document
local FS = window.FS

local future = async(function (self)
  FS:mkdir "/save"
  FS:mount(D.window.IDBFS, {}, "/save")

  self:await(function (self)
    FS:syncfs(true, function (e)
      self:resume(D.is_falsy(e), e)
    end)
  end)

  print "unlink test.txt"
  local status, result = pcall(function () FS:unlink "/save/test.txt" end)
  if not status then
    io.stderr:write(("cannot unlink %s: %s\n"):format(path, result))
  end

  self:await(function (self)
    FS:syncfs(function (e)
      self:resume(D.is_falsy(e), e)
    end)
  end)

  FS:unmount "/save"

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
