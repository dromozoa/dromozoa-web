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
local subtle = G.crypto.subtle
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

  local pair = await(subtle:generateKey({ name = "ECDSA", namedCurve = "P-256" }, true, D.array { "sign", "verify" }))
  local private_key = pair.privateKey
  local public_key = pair.publicKey
  local public_key_jwk = G.JSON:stringify(await(subtle:exportKey("jwk", public_key)))
  print(public_key_jwk)

  local nonce = G.crypto:randomUUID()
  print(nonce)

  local open = document:createElement "button" :append "Open"
  open:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    if socket then
      io.stderr:write "socket not closed\n"
      return
    end

    futures:async(function ()
      local url = D.new(G.URL, "wss://n2qtecb02e.execute-api.ap-northeast-1.amazonaws.com/d")
      url.searchParams:append("name", "testだよ")
      url.searchParams:append("nonce", nonce)
      url.searchParams:append("public_key", public_key_jwk)
      print(url)
      url = aws.sign_query(access_key, secret_key, "GET", url, {})
      print(url)

      socket = D.new(G.WebSocket, url)

      socket.onopen = function ()
        print "onopen"
        socket:send [[{"action":"get_connection"}]]
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
  end)

  local get = document:createElement "button" :append "Get"
  get:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    if not socket then
      io.stderr:write "socket not opened\n"
      return
    end

    socket:send [[{"action":"get_connection"}]]
  end)

  local put = document:createElement "button" :append "Put"
  put:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()

    if not socket then
      io.stderr:write "socket not opened\n"
      return
    end

    socket:send [[{"action":"put_connection"}]]
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

  local get_connection = document:createElement "button" :append "GET /connections/{connection_id}"
  get_connection:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url = D.new(G.URL, "https://iuhpbxsimf.execute-api.ap-northeast-1.amazonaws.com/d/connections/" .. key)
      local headers = aws.sign(access_key, secret_key, "GET", url, {})
      local response = await(G:fetch(url, { cache = "no-store", headers = headers }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  local get_connections = document:createElement "button" :append "GET /connections"
  get_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url = D.new(G.URL, "https://iuhpbxsimf.execute-api.ap-northeast-1.amazonaws.com/d/connections")
      local headers = aws.sign(access_key, secret_key, "GET", url, {})
      local response = await(G:fetch(url, { cache = "no-store", headers = headers }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  local get_socket_connections = document:createElement "button" :append "GET /socket_connections/{connection_id}"
  get_socket_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url1 = D.new(G.URL, "https://n2qtecb02e.execute-api.ap-northeast-1.amazonaws.com/d/@connections/" .. key)
      local url2 = D.new(G.URL, "https://iuhpbxsimf.execute-api.ap-northeast-1.amazonaws.com/d/socket_connections/" .. key)
      local headers1 = aws.sign(access_key, secret_key, "GET", url1, {})
      headers1:set("x-dromozoa-web-socket-connections-authorization", headers1:get "authorization")
      headers1:delete "authorization"
      local headers2 = aws.sign(access_key, secret_key, "GET", url2, headers1)
      local response = await(G:fetch(url2, { cache = "no-store", headers = headers2 }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  local post_socket_connections = document:createElement "button" :append "POST /socket_connections/{connection_id}"
  post_socket_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url1 = D.new(G.URL, "https://n2qtecb02e.execute-api.ap-northeast-1.amazonaws.com/d/@connections/" .. key)
      local url2 = D.new(G.URL, "https://iuhpbxsimf.execute-api.ap-northeast-1.amazonaws.com/d/socket_connections/" .. key)
      local body = [[{"foo":42}]]
      local headers1 = aws.sign(access_key, secret_key, "POST", url1, {}, body)
      headers1:set("x-dromozoa-web-socket-connections-authorization", headers1:get "authorization")
      headers1:delete "authorization"
      local headers2 = aws.sign(access_key, secret_key, "POST", url2, headers1, body)
      local response = await(G:fetch(url2, { method = "POST", cache = "no-store", headers = headers2, body = body }))
      if response.ok then
        local body = await(response:text())
        print(body)
      else
        io.stderr:write(("cannot fetch %s: %d %s\n"):format(url, response.status, response.statusText))
      end
    end)
  end)

  local delete_socket_connections = document:createElement "button" :append "DELETE /socket_connections/{connection_id}"
  delete_socket_connections:addEventListener("click", function (ev)
    ev:preventDefault()
    ev:stopPropagation()
    futures:async(function ()
      local key = connection_id.value
      local url1 = D.new(G.URL, "https://n2qtecb02e.execute-api.ap-northeast-1.amazonaws.com/d/@connections/" .. key)
      local url2 = D.new(G.URL, "https://iuhpbxsimf.execute-api.ap-northeast-1.amazonaws.com/d/socket_connections/" .. key)
      local headers1 = aws.sign(access_key, secret_key, "DELETE", url1, {})
      headers1:set("x-dromozoa-web-socket-connections-authorization", headers1:get "authorization")
      headers1:delete "authorization"
      local headers2 = aws.sign(access_key, secret_key, "DELETE", url2, headers1)
      local response = await(G:fetch(url2, { method = "DELETE", cache = "no-store", headers = headers2 }))
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
    :append(document:createElement "div" :append(get))
    :append(document:createElement "div" :append(put))
    :append(document:createElement "div" :append(close))
    :append(document:createElement "div" :append(connection_id))
    :append(document:createElement "div" :append(get_connection))
    :append(document:createElement "div" :append(get_connections))
    :append(document:createElement "div" :append(get_socket_connections))
    :append(document:createElement "div" :append(post_socket_connections))
    :append(document:createElement "div" :append(delete_socket_connections))
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
