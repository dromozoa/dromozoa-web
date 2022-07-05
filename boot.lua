R""--(
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

local thread = coroutine.create(function ()
  local window = D.window
  local document = window.document

  local filename = D.new(window.URLSearchParams, document.location.search):get "main"
  if not filename or filename == D.null then
    filename = "main.lua"
  end

  local result
  local data

  window:fetch(filename, { cache = "no-store" })
    :then_(function (response)
      if response.ok then
        return response:text()
      else
        D.throw(("%d %s"):format(response.status, response.statusText))
      end
    end)
    :then_(function (text)
      result = true
      data = text
    end)
    :catch(function (e)
      result = false
      data = e.message
    end)

  while true do
    while true do
      local e = D.pop_error_queue()
      if not e then
        break
      end
      io.stderr:write(e, "\n")
    end
    if result ~= nil then
      break
    end
    coroutine.yield()
  end

  if not result then
    error(data)
  end

  return assert(load(data, "@" .. filename))()
end)

return function ()
  if not thread then
    return
  end

  local result, data = coroutine.resume(thread)
  if coroutine.status(thread) == "dead" then
    thread = nil
  end

  if not result then
    error(data)
  end
end
--)"--"
