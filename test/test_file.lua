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

local FS = D.window.FS

local thread = coroutine.create(function (thread)
  FS:mkdir "/save"
  FS:mount(D.window.IDBFS, {}, "/save")
  FS:syncfs(true, function (e)
    print("syncfs callback", e)
    assert(coroutine.resume(thread, e))
  end)

  local e = coroutine.yield(thread)
  print("?", e)
  if e and e ~= D.null then
    return
  end

  FS:chdir "/save"

  if true then
    local out = assert(io.open("test.txt", "w"))
    out:write [[
日本語
日本語
日本語
]]
    out:close()
  end

  if true then
    local handle, message = io.open "test.txt"
    if handle then
      print(handle:read "*a")
      handle:close()
    else
      print("cannot open", message)
    end
  end

  if false then
    D.window:setTimeout(function ()
      print "timeout"
      assert(coroutine.resume(thread))
    end, 1000)

    coroutine.yield(thread)
  end

  FS:syncfs(function (e)
    print("syncfs callback", e)
    assert(coroutine.resume(thread, e))
  end)

  coroutine.yield(thread)
  FS:unmount("/save")
end)
assert(coroutine.resume(thread, thread))

while true do
  assert(D.get_error_queue())
  coroutine.yield()
end
