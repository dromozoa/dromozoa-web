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

local future = async(function (self)
  local v = await(function (self)
    self:resume(true, D.new(window.Number, 42))
  end)

  assert(v ~= 42)
  assert(v:valueOf() == 42)
  assert(type(v) == "userdata")
  assert(D.typeof(v) == "object")
  assert(D.instanceof(v, window.Object))
  assert(D.instanceof(v, window.Number))
  assert(not D.instanceof(v, window.Promise))

  local n = 0
  local v = await(function (self)
    self:resume(true, D.ref(function (ev)
      n = n + 1
      print("event", n)
    end))
  end)

  assert(n == 0)
  assert(type(v) == "userdata")
  assert(D.typeof(v) == "function")
  assert(D.instanceof(v, window.Object))
  assert(D.instanceof(v, window.Function))
  assert(not D.instanceof(v, window.Promise))

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

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end
  coroutine.yield()
end
