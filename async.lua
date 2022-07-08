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

--[[

local async, await = require "dromozoa.web.async" :export()

]]

local class = {
  queue = { min = 1, max = 0 };
  map = setmetatable({}, { __mode = "k" });
}

function class.delay(fn)
  local queue = class.queue
  local max = queue.max + 1
  queue[max] = fn
  queue.max = max
end

local promise = {}
local promise_metatable = {
  __index = promise;
  __name = "dromozoa.web.async.promise";
}

local function promise_resume(self, ...)
  local thread = self.thread
  self.status = "running"
  local result = table.pack(coroutine.resume(thread, ...))
  if coroutine.status(thread) == "dead" then
    -- 終了フックがあったら、それを呼び出す
    self.status = "ready"
    self.thread = nil
    self.result = result
    class.map[thread] = nil
  else
    assert(table.unpack(result, 1, result.n))
  end
end

function promise:set(...)
  if coroutine.status(self.thread) == "suspended" then
    promise_resume(self, ...)
  else
    local args = table.pack(...)
    class.delay(function () promise_resume(self, table.unpack(args, 1, args.n)) end)
  end
end

local future = {}
local future_metatable = {
  __index = future;
  __name = "dromozoa.web.async.future";
}

function future:is_ready()
  return self.promise.status == "ready"
end

function future:get()
  local result = assert(self.promise.result)
  self.promise.result = nil
  if result[1] then
    return table.unpack(result, 2, result.n)
  else
    error(result[2])
  end
end

local function async(fn)
  local thread = coroutine.create(function () return fn() end)
  local promise = setmetatable({ status = "initial", thread = thread }, promise_metatable)
  class.map[thread] = promise
  promise_resume(promise)
  return setmetatable({ promise = promise }, future_metatable)
end

function class.await(that)
  local thread, is_main_thread = coroutine.running()
  assert(not is_main_thread)
  local promise = assert(class.map[thread])

  if D.instanceof(that, D.window.Promise) then
    that["then"](that, function (...)
      promise:set(true, ...)
    end):catch(function (...)
      that:set(false, ...)
    end)
  else
    that(promise)
  end

  promise.status = "suspended"
  local result = table.pack(coroutine.yield())
  if result[1] then
    return table.unpack(result, 2, result.n)
  else
    error(result[2])
  end
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

function class.export()
  return class, class.await
end

return setmetatable(class, {
  __call = function (_, fn)
    return async(fn)
  end;
})

--)]]--"
