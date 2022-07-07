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

local future = async(function (self)
  local window = D.window
  local FS = window.FS

  print "mkdir /save"
  FS:mkdir "/save"
  print "mount IDBFS /save"
  FS:mount(D.window.IDBFS, {}, "/save")

  print "sync true"
  self:await(function (self)
    FS:syncfs(true, function (e)
      print(e)
      if not e or e == D.null then
        self:resume(true)
      else
        self:resume(false, e)
      end
    end)
  end)

  print "chdir /save"
  FS:chdir "/save"

  print "open test.txt"
  local out = assert(io.open("test.txt", "w"))
  print "write test.txt"
  out:write(os.date "%Y-%m-%d %H:%M:%S", "\n")
  print "close test.txt"
  assert(out:close())

  print "unmount /save"
  FS:unmount "/save"

  print "sync false"
  self:await(function (self)
    FS:syncfs(function (e)
      print(e)
      if not e or e == D.null then
        self:resume(true)
      else
        self:resume(false, e)
      end
    end)
  end)
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end

  assert(D.get_error_queue())
  coroutine.yield()
end
