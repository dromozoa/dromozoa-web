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

local function unpack(t, i, j)
  return table.unpack(t, i or 1, j or t.n)
end

local promise = {}
local promise_metatable = { __index = promise, __name = "dromozoa.web.async.promise" }
local promise_map = setmetatable({}, { __mode = "k" })

local function promise_resume(self, ...)
  local thread = assert(self.thread)
  local result = table.pack(coroutine.resume(thread, ...))
  if coroutine.status(thread) == "dead" then
    self.thread = nil
    promise_map[thread] = nil

    local chain = self.chain
    if chain then
      promise_resume(chain, unpack(result))
    else
      self.result = result
    end
  else
    assert(unpack(result))
  end
end

local function promise_new(fn)
  local thread = coroutine.create(function () return fn() end)
  local self = setmetatable({ thread = thread }, promise_metatable)
  promise_map[thread] = self
  promise_resume(self)
  return self
end

local function promise_is_ready(self)
  return not self.thread
end

local function promise_get(self)
  local result = assert(self.result)
  self.result = nil
  return result
end

local function promise_chain(self, chain)
  assert(not self.chain)
  self.chain = chain
  if promise_is_ready(self) then
    delay(function () promise_resume(chain, unpack(promise_get(self))) end)
  end
end

function promise:set(...)
  if coroutine.status(self.thread) == "suspended" then
    promise_resume(self, ...)
  else
    local args = table.pack(...)
    delay(function () promise_resume(self, unpack(args)) end)
  end
end

local future = {}
local future_metatable = { __index = future, __name = "dromozoa.web.async.future" }

local function future_new(promise)
  return setmetatable({ promise }, future_metatable)
end

local function future_get_promise(self)
  return self[1]
end

-- function future:is_valid()
--   return self[1].result
-- end

function future:is_ready()
  return promise_is_ready(future_get_promise(self))
end

function future:get()
  local result = promise_get(future_get_promise(self))
  if result[1] then
    return table.unpack(result, 2, result.n)
  else
    error(result[2])
  end
end

local class = { delay = delay }

function class.import(list)
  return class, class.await
end

function class.await(that)
  local thread, is_main_thread = coroutine.running()
  assert(not is_main_thread)
  local promise = assert(promise_map[thread])

  if getmetatable(that) == future_metatable then
    promise_chain(future_get_promise(that), promise)
  elseif D.instanceof(that, D.window.Promise) then
    that["then"](that, function (...)
      promise:set(true, ...)
    end):catch(function (...)
      promise:set(false, ...)
    end)
  else
    that(promise)
  end

  local result = table.pack(coroutine.yield())
  if result[1] then
    return unpack(result, 2)
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

return setmetatable(class, {
  __call = function (_, fn)
    return future_new(promise_new(fn))
  end;
})

--)]]--"
