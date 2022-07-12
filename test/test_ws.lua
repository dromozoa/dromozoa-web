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

local D, G = require "dromozoa.web" :import "global"
local async, await = require "dromozoa.web.async" :import "await"

local FS = G.FS
local document = G.document
local socket

local futures = { n = 0 }

function futures:async(fn)
  local n = self.n + 1
  self.n = n
  self[n] = async(fn)
end

futures:async(function ()
  print "started"

  local aws = async.require "dromozoa.web.aws"

  FS:mkdir "/save"
  FS:mount(G.IDBFS, {}, "/save")

  await(function (promise)
    FS:syncfs(true, function (e)
      promise:set(D.is_falsy(e), e)
    end)
  end)

  local access_key = ""
  local secret_key = ""

  local credentials_path = "/save/credentials"
  local handle, message = io.open(credentials_path)
  if handle then
    access_key = handle:read "*l"
    secret_key = handle:read "*l"
    assert(handle:close())
  else
    io.stderr:write(("cannot open %s: %s\n"):format(credentials_path, message))
  end

  local open = document:createElement "button" :append "Open"
  open:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    if socket then
      io.stderr:write "socket not closed\n"
      return
    end

    socket = D.new(G.WebSocket, "wss://ce57u4bdl6.execute-api.ap-northeast-1.amazonaws.com/d?name=foo-server")

    socket.onopen = function ()
      print "onopen"

      socket:send [[{}]]
    end

    socket.onclose = function ()
      print "onclose"
      socket = nil
    end

    socket.onerror = function (ev)
      print("onerror", ev, ev.message)
    end

    socket.onmessage = function (ev)
      print("onmessage", ev, ev.data)
    end
  end)

  local close = document:createElement "button" :append "Close"
  close:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    if not socket then
      io.stderr:write "socket not opened\n"
      return
    end

    socket:close()
  end)

  local connection_id = document:createElement "input" :setAttribute("type", "text")

  local get_connections = document:createElement "button" :append "GET @connections"
  get_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url1 = D.new(G.URL, "https://ce57u4bdl6.execute-api.ap-northeast-1.amazonaws.com/d/@connections/" .. key)
      local url2 = D.new(G.URL, "https://ycyu9ow4k0.execute-api.ap-northeast-1.amazonaws.com/d/connections/" .. key)
      local headers = aws.sign(access_key, secret_key, "GET", url1, {})
      headers:set("x-dromozoa-web-authorization", headers:get "authorization")
      local response = await(G:fetch(url2, { cache = "no-store", headers = headers }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  local post_connections = document:createElement "button" :append "POST @connections"
  post_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url1 = D.new(G.URL, "https://ce57u4bdl6.execute-api.ap-northeast-1.amazonaws.com/d/@connections/" .. key)
      local url2 = D.new(G.URL, "https://ycyu9ow4k0.execute-api.ap-northeast-1.amazonaws.com/d/connections/" .. key)
      local body = [[{"foo":42}]]
      local headers = aws.sign(access_key, secret_key, "POST", url1, {}, body)
      headers:set("x-dromozoa-web-authorization", headers:get "authorization")
      local response = await(G:fetch(url2, { method = "POST", cache = "no-store", headers = headers, body = body }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  document.body:append(document:createElement "div"
    :append(document:createElement "div" :append(open))
    :append(document:createElement "div" :append(close))
    :append(document:createElement "div" :append(connection_id))
    :append(document:createElement "div" :append(get_connections))
    :append(document:createElement "div" :append(post_connections))
  )

  print "finished"
end)

while true do
  for k, future in pairs(futures) do
    if type(future) == "table" and future.is_ready and future:is_ready() then
      future:get()
      futures[k] = nil
    end
  end
  coroutine.yield()
end
