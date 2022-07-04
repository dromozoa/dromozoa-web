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

local class = {}
local metatable = { __index = class }

function class.new(f)
  return setmetatable({ thread = coroutine.create(f) }, metatable)
end

function class:await(p)
  p["then"](p, function (v)
    assert(coroutine.resume(self.thread, true, v))
  end):catch(function (v)
    assert(coroutine.resume(self.thread, false, v))
  end)
  -- then,catchのなかで実行されていることに注意
  local result, v = coroutine.yield()
  if result then
    return v
  else
    self:error(v)
  end
end

function class:error(v)
  error(v)
end

local function async(f)
  local self = class.new(f)
  assert(coroutine.resume(self.thread, self))
end

async(function (self)
  local url = "https://nozomi.dromozoa.com/"
  -- local url = "main.lua"
  local response = self:await(D.window:fetch(url, { cache = "no-store" }))
  if response.ok then
    local text = self:await(response:text())
    print("ok", url)
  else
    self:error("cannot fetch: " .. url)
  end
end)

async(function (self)
  local url = "index.js"
  local response = self:await(D.window:fetch(url, { cache = "no-store" }))
  if response.ok then
    local text = self:await(response:text())
    print("ok", url)
  else
    error("cannot fetch: " .. url)
  end
end)

while true do
  while true do
    local e = D.get_error()
    if not e then
      break
    end
    io.stderr:write(e, "\n")
  end

  coroutine.yield()
end
