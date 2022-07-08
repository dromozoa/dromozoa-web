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

local delay_queue = { min = 1, max = 0 }

local function delay(fn)
  local max = delay_queue.max + 1
  delay_queue[max] = fn
  delay_queue.max = max
end

local promise = {}
local promise_metatable = { __index = promise, __name = "dromozoa.web.async.promise" }
local promise_map = setmetatable({}, { __mode = "k" })

local function promise_resume(self, ...)
  local thread = self.thread
  self.status = "running"
  local result = table.pack(coroutine.resume(thread, ...))
  if coroutine.status(thread) == "dead" then
    -- 終了フックがあったら、それを呼び出す
    if self.parent then
      self.status = "ready"
      self.thread = nil
      self.result = nil
      promise_map[thread] = nil
      promise_resume(self.parent, table.unpack(result, 1, result.n))
    else
      self.status = "ready"
      self.thread = nil
      self.result = result
      promise_map[thread] = nil
    end
  else
    assert(table.unpack(result, 1, result.n))
  end
end

local function promise_new(fn)
  local thread = coroutine.create(function () return fn() end)
  local self = setmetatable({ status = "initial", thread = thread }, promise_metatable)
  promise_map[thread] = self
  promise_resume(self)
  return self
end

local function promise_set_parent(self, parent)
  assert(not self.parent)
  self.parent = parent
  if self.status == "ready" then
    delay(function ()
      -- local thread = assert(self.thread)
      -- self.status = "ready"
      -- self.thread = nil
      -- self.result = nil
      -- promise_map[thread] = nil
      local result = self.result
      promise_resume(self.parent, table.unpack(result, 1, result.n))
    end)
  end
end

function promise:set(...)
  if coroutine.status(self.thread) == "suspended" then
    promise_resume(self, ...)
  else
    local args = table.pack(...)
    delay(function () promise_resume(self, table.unpack(args, 1, args.n)) end)
  end
end

local future = {}
local future_metatable = { __index = future, __name = "dromozoa.web.async.future" }

local function future_new(promise)
  return setmetatable({ promise }, future_metatable)
end

function future:is_ready()
  return self[1].status == "ready"
end

function future:get()
  local result = assert(self[1].result)
  self[1].result = nil
  if result[1] then
    return table.unpack(result, 2, result.n)
  else
    error(result[2])
  end
end

local class = { delay = delay }

function class.await(that)
  local thread, is_main_thread = coroutine.running()
  assert(not is_main_thread)
  local promise = assert(promise_map[thread])

  if getmetatable(that) == future_metatable then
    -- print("promise_set_parent", that, that[1], promise)
    promise_set_parent(that[1], promise)
  elseif D.instanceof(that, D.window.Promise) then
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
  for i = delay_queue.min, delay_queue.max do
    local fn = delay_queue[i]
    delay_queue.min = i + 1
    fn()
  end
  if delay_queue.min > delay_queue.max then
    delay_queue.min = 1
    delay_queue.max = 0
  end
end

function class.export()
  return class, class.await
end

return setmetatable(class, {
  __call = function (_, fn)
    return future_new(promise_new(fn))
  end;
})

--)]]--"
