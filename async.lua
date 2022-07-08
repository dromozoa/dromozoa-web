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
  tasks = {};
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

function class:resume(...)
  local args = table.pack(...)

  local fn = function ()
    local thread = assert(self.thread)
    self.status = "running"
    local result = table.pack(coroutine.resume(thread, table.unpack(args, 1, args.n)))
    if coroutine.status(thread) == "dead" then
      self.status = "ready"
      self.thread = nil
      self.result = result
    else
      assert(table.unpack(result, 1, result.n))
    end
  end

  local thread = assert(self.thread)
  if coroutine.status(thread) == "suspended" then
    fn()
  else
    class.push_task(fn)
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

function class.push_task(fn)
  local tasks = class.tasks
  tasks[#tasks + 1] = fn
end

function class.process_tasks()
  if #class.tasks > 0 then
    local tasks = {}
    tasks, class.tasks = class.tasks, tasks
    for i = 1, #tasks do
      tasks[i]()
    end
  end
end

return setmetatable(class, {
  __call = function (_, fn)
    return new(fn)
  end;
})

--)]]--"
