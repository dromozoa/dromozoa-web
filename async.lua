#if 0
--[[
#endif
"\n\n\n" R"]]--(
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

local class = {
  queue = {
    min = 1;
    max = 0;
  };
}

local metatable = {
  __index = class;
  __name = "dromozoa.web.async";
}

local function new(fn)
  local self = setmetatable({
    status = "initial";
    thread = coroutine.create(function (self)
      return fn(self)
    end);
  }, metatable)
  self:resume(self)
  return self
end

local function resume(self, ...)
  local thread = self.thread
  self.status = "running"
  local result = table.pack(coroutine.resume(thread, ...))
  if coroutine.status(thread) == "dead" then
    self.status = "ready"
    self.thread = nil
    self.result = result
  else
    assert(table.unpack(result, 1, result.n))
  end
end

function class:resume(...)
  if coroutine.status(self.thread) == "suspended" then
    resume(self, ...)
  else
    local args = table.pack(...)
    class.delay(function ()
      resume(self, table.unpack(args, 1, args.n))
    end)
  end
end

function class:yield()
  self.status = "suspended"
  return table.pack(coroutine.yield())
end

function class:await(that)
  if D.instanceof(that, D.window.Promise) then
    that["then"](that, function (...)
      self:resume(true, ...)
    end):catch(function (...)
      that:resume(false, ...)
    end)
  else
    that(self)
  end
  local result = self:yield()
  if result[1] then
    return table.unpack(result, 2, result.n)
  else
    error(result[2])
  end
end

function class:is_ready()
  return self.status == "ready"
end

function class:get(fn)
  local result = assert(self.result)
  self.result = nil
  if result[1] then
    return table.unpack(result, 2)
  else
    (fn or error)(result[2])
  end
end

function class.delay(fn)
  local queue = class.queue
  local max = queue.max + 1
  queue[max] = fn
  queue.max = max
end

function class.dispatch()
  local queue = class.queue
  for i = queue.min, queue.max do
    local fn = queue[i]
    queue.min = i + 1
    fn()
  end
  if queue.min > queue.max then
    queue.min = 1
    queue.max = 0
  end
end

return setmetatable(class, {
  __call = function (_, fn)
    return new(fn)
  end;
})

--)]]--"
