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

local prototype = D.window.Promise.prototype
prototype.then_ = prototype["then"]

local function fetch(url, thread)
  D.window:fetch(url, { cache = "no-store" })
  :then_(function (response)
    print("then1", url, response.ok)
    if response.ok then
      return response:text()
    else
      D.throw "!!!"
    end
  end)
  :then_(function (text)
    print("then2", url)
    coroutine.resume(thread, "success", url, text)
  end)
  :catch(function (e)
    print("catch", url, e.message)
    coroutine.resume(thread, "failure", url, e)
  end)
  return thread
end

local t1 = fetch("https://honoka.dromozoa.com/", coroutine.create(function (result, url, v) print(result, url) end))
local t2 = fetch("prologue.js", coroutine.create(function (result, url, v) print(result, url) end))
local t3 = fetch("no-such-file.txt", coroutine.create(function (result, url, v) print(result, url) end))
local t4 = fetch("README.md", coroutine.create(function (result, url, v) print(result, url) end))

warn "@on"
warn "WARN"
warn "あいうえお"

D.window:setTimeout(function () error "die1" end, 1000)
D.window:setTimeout(function () error "die2" end, 1000)
D.window:setTimeout(function () error "die3" end, 1000)

while true do
  assert(D.get_error_queue())
  coroutine.yield()
end
