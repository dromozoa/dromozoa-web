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

local D, G = require "dromozoa.web" .import "global"
local async, await = require "dromozoa.web.async" .import "await"

local document = G.document

local future = async(function ()
  local v = await(function (promise)
    promise:set(true, D.new(G.Number, 42))
  end)

  assert(v ~= 42)
  assert(v:valueOf() == 42)
  assert(type(v) == "userdata")
  assert(D.typeof(v) == "object")
  assert(D.instanceof(v, G.Object))
  assert(D.instanceof(v, G.Number))
  assert(not D.instanceof(v, G.Promise))

  local n = 0
  local v = await(function (promise)
    promise:set(true, D.ref(function (ev)
      n = n + 1
      print("event", n)
    end))
  end)

  assert(n == 0)
  assert(type(v) == "userdata")
  assert(D.typeof(v) == "function")
  assert(D.instanceof(v, G.Object))
  assert(D.instanceof(v, G.Function))
  assert(not D.instanceof(v, G.Promise))

  local button1 = document:createElement "button" :append "ボタン1"
  document.body:append(button1)

  local flag
  local button2 = document:createElement "button" :append "ボタン2"
  button2:addEventListener("click", function (ev)
    flag = not flag
    if flag then
      print "ボタン1にイベントリスナを追加"
      button1:addEventListener("click", v)
    else
      print "ボタン1のイベントリスナを削除"
      button1:removeEventListener("click", v)
    end
  end)
  document.body:append(button2)

  assert(not D.is_truthy(false))
  assert(not D.is_truthy(0))
  assert(not D.is_truthy(0.0))
  assert(not D.is_truthy(-0.0))
  assert(not D.is_truthy "")
  assert(not D.is_truthy(D.null))
  assert(not D.is_truthy(nil))
  assert(not D.is_truthy())
  assert(not D.is_truthy(0/0)) -- nan

  assert(D.is_truthy(true))
  assert(D.is_truthy(1))
  assert(D.is_truthy "foo")
  assert(D.is_truthy(1/0)) -- inf

  assert(D.is_falsy(false))
  assert(D.is_falsy(0))
  assert(D.is_falsy(0.0))
  assert(D.is_falsy(-0.0))
  assert(D.is_falsy "")
  assert(D.is_falsy(D.null))
  assert(D.is_falsy(nil))
  assert(D.is_falsy())
  assert(D.is_falsy(0/0)) -- nan

  assert(not D.is_falsy(true))
  assert(not D.is_falsy(1))
  assert(not D.is_falsy "foo")
  assert(not D.is_falsy(1/0)) -- inf

  local v = coroutine.running()
  assert(D.typeof(v) == nil)
  assert(not D.instanceof(v, G.Object))
  assert(D.is_truthy(v))
  assert(not D.is_falsy(v))

  local v = { number = 42, thread = coroutine.running() }
  assert(D.ref(v).number == 42)
  assert(D.ref(v).thread == nil)
  assert(D.typeof(v) == "object")
  assert(D.instanceof(v, G.Object))
  assert(D.is_truthy(v))
  assert(not D.is_falsy(v))

  local v = D.new(G.Array, 17, undefined, 42)
  local a, b, c = D.unpack(v)
  assert(a == 17)
  assert(b == nil)
  assert(c == 42)

  local buffer = {}
  for _, item in D.each(v:entries()) do
    local k, v = D.unpack(item)
    buffer[k + 1] = v
  end
  assert(buffer[1] == 17)
  assert(buffer[2] == nil)
  assert(buffer[3] == 42)

  print(G.Symbol)
  print(G.Symbol.iterator)
  print(D.typeof(G.Symbol.iterator))
  local symbol = G.Symbol.iterator
  print(v[symbol])

  -- for _, item in D.each(v[G.Symbol.iterator]) do
  --   print(item)
  -- end

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
