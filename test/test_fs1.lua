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
local document = window.document
local FS = window.FS

local function readdir(parent_path, element)
  local entries = FS:readdir(parent_path)
  for i = 0, entries.length - 1 do
    local entry = entries[i]
    if entry ~= "." and entry ~= ".." then
      local path
      if parent_path == "/" then
        path = "/" .. entry
      else
        path = parent_path .. "/" .. entry
      end
      element:append(document:createElement "li":append(path))

      local status, result = pcall(function () return FS:stat(path) end)
      if status then
        if FS:isDir(result.mode) then
          readdir(path, element)
        end
      else
        io.stderr:write(("cannot stat %s: %s\n"):format(path, result))
      end
    end
  end
end

local future = async(function ()
  print "mkdir /save"
  FS:mkdir "/save"
  print "mount IDBFS /save"
  FS:mount(D.window.IDBFS, {}, "/save")

  print "sync true"
  await(function (promise)
    FS:syncfs(true, function (e)
      promise:set(D.is_falsy(e), e)
    end)
  end)

  local ul = document:createElement "ul"
  ul:append(document:createElement "li":append "/")
  readdir("/", ul)
  document.body:append(ul)

  local counter = 0

  print "read test.txt"
  local path = "/save/test.txt"
  local handle, result = io.open(path)
  if handle then
    local content = handle:read "*a"
    print(content)
    handle:close()
    local n = content:match "^(%d+)\n"
    if n then
      counter = tonumber(n) + 1
    end
  else
    io.stderr:write(("cannot open %s: %s\n"):format(path, result))
  end

  print "write test.txt"
  local out = assert(io.open("/save/test.txt", "w"))
  out:write(counter, "\n", os.date "%Y-%m-%d %H:%M:%S", "\n")
  assert(out:close())

  print "sync"
  await(function (promise)
    FS:syncfs(function (e)
      promise:set(D.is_falsy(e), e)
    end)
  end)

  print "unmount /save"
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
