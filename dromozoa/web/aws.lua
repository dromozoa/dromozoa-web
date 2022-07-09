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
local window = D.window
local subtle = window.crypto.subtle
local async, await = require "dromozoa.web.async" .import "await"

local class = {}

local function array_buffer(that)
  if type(that) == "string" then
    return D.slice(that)
  elseif D.instanceof(that, window.Blob) then
    return await(that:arrayBuffer())
  else
    return that
  end
end

function class.hex(data)
  local source = D.new(window.Uint8Array, data)
  local buffer = {}
  for i = 1, source.length do
    buffer[i] = ("%02x"):format(source[i - 1])
  end
  return table.concat(buffer)
end

function class.sha256(data)
  return await(subtle:digest("SHA-256", array_buffer(data)))
end

function class.hmac_sha256(key, data)
  return await(subtle:sign("HMAC", await(subtle:importKey("raw", array_buffer(key), { name = "HMAC", hash = { name = "SHA-256"} }, false, D.array { "sign" })), array_buffer(data)))
end

function class.get_signature_key(secret_key, date, region, service)
  return class.hmac_sha256(class.hmac_sha256(class.hmac_sha256(class.hmac_sha256("AWS4" .. secret_key, date), region), service), "aws4_request")
end

function class.sign(source)
  -- window.console:log(source)
  local url = D.new(window.URL, source.url)
  window.console:log(url)

  local iterator = source.headers:entries()
  local entry = iterator:next()
  while not entry.done do
    local kv = entry.value
    print(kv[0], kv[1])
    entry = iterator:next()
  end

end

return class
