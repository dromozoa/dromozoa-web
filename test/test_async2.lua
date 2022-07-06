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

----------------------------------------------------------------------

local class = {}
local metatable = {
  __index = class;
  __name = "dromozoa.web.async";
}

function class:resume(...)
  local thread = self.thread
  -- 死んだスレッドで処理を続けようとしているかもしれない
  if thread then
    local result = table.pack(coroutine.resume(thread, ...))
    if coroutine.status(thread) == "dead" then
      self.thread = nil
      self.value = result
      self.status = "ready"
    end
  end
end

function class:await(promise)
  promise["then"](promise, function (...)
    self:resume(true, ...)
  end):catch(function (...)
    self:resume(false, ...)
  end)
  return coroutine.yield()
end

function class:is_ready()
  return self.status == "ready"
end

function class:get()
  local value = self.value
  if value[1] then
    return table.unpack(value, 2)
  else
    error(value[2])
  end
end

local async = setmetatable(class, {
  __call = function (_, f)
    local self = setmetatable({
      status = "initial";
      thread = coroutine.create(function (self)
        self.status = "running"
        print("f", f)
        return f(self)
      end);
    }, metatable)
    self:resume(self)
    return self
  end;
})

----------------------------------------------------------------------

local function timeout_promise(t, k)
  return D.new(D.window.Promise, function (resolve, reject)
    D.window:setTimeout(function ()
      resolve(nil, k)
    end, t)
  end)
end

local a = async(function (self)
  print "start"
  local p = timeout_promise(500, "A")
  print(D.typeof(p), D.typeof(42), D.typeof(function () end))
  print(D.instanceof(p, D.window.Object), D.instanceof(p, D.window.Promise), D.instanceof(p, D.window.Array))
  print(D.instanceof(42, D.window.Object), D.instanceof(42, D.window.Promise), D.instanceof(42, D.window.Array))
  print(self:await(p))
  print(self:await(timeout_promise(500, "B")))
  print(self:await(timeout_promise(500, "C")))
  -- error "X"
  print(self:await(timeout_promise(500, "D")))
  print "goal"
  return "z"
end)

while true do
  assert(D.get_error_queue())

  if a and a:is_ready() then
    print "ready"
    print(a:get())
    a = nil
  end

  coroutine.yield()
end
