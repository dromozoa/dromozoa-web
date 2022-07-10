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

local subtle = G.crypto.subtle

local class = {}

local function array_buffer(that)
  if type(that) == "string" then
    return D.slice(that)
  elseif D.instanceof(that, G.Blob) then
    return await(that:arrayBuffer())
  else
    return that
  end
end

function class.hex(data)
  local source = D.new(G.Uint8Array, data)
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
  -- G.console:log(source)
  local url = D.new(G.URL, source.url)
  G.console:log(url)

  for i, item in D.each(source.headers:entries()) do
    local k, v = D.unpack(item)
    print(i, k, v)
  end
end

return class
