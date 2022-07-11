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

local subtle = G.crypto.subtle

local function array_buffer(that)
  if type(that) == "string" then
    return D.slice(that)
  elseif D.instanceof(that, G.Blob) then
    return await(that:arrayBuffer())
  else
    return that
  end
end

local function compare(a, b)
  local x = a[1]
  local y = b[1]
  if x == y then
    return a[2] < b[2]
  else
    return x < y
  end
end

local function trim(s)
  return (s:gsub("^%s+", ""):gsub("%s+$", ""))
end

local uri_encoder = {}
for byte = 0x00, 0xFF do
  uri_encoder[string.char(byte)] = ("%%%02X"):format(byte)
end

local function uri_encode(s)
  return (s:gsub("[^A-Za-z0-9%-%_%.%~]", uri_encoder))
end

local function uri_encode_path(s)
  return (s:gsub("[^A-Za-z0-9%-%_%.%~%/]", uri_encoder))
end

local hex_encoder = {}
for byte = 0x00, 0xFF do
  hex_encoder[byte] = ("%02x"):format(byte)
end

local function hex(data)
  local source = D.new(G.Uint8Array, data)
  local buffer = {}
  for i = 1, source.length do
    buffer[i] = hex_encoder[source[i - 1]]
  end
  return table.concat(buffer)
end

local function sha256(data)
  return await(subtle:digest("SHA-256", array_buffer(data)))
end

local function hmac_sha256(key, data)
  return await(subtle:sign("HMAC", await(subtle:importKey("raw", array_buffer(key), { name = "HMAC", hash = { name = "SHA-256"} }, false, D.array { "sign" })), array_buffer(data)))
end

local function get_signature_key(secret_key, date, region, service)
  return hmac_sha256(hmac_sha256(hmac_sha256(hmac_sha256("AWS4" .. secret_key, date), region), service), "aws4_request")
end

local class = {}

function class.sign(access_key, secret_key, method, url, headers, body)
  local url = D.new(G.URL, url)
  local headers = D.new(G.Headers, headers)

  -- https://github.com/boto/botocore/blob/develop/botocore/data/endpoints.json
  local host = url.host
  local service, region = host:match "([^%.]+)%.(%a%a%-%w+%-%d+)%.amazonaws%.com"
  if not service then
    service = host:match "([^%.]+)%.amazonaws.com"
    region = "us-east-1"
  end
  assert(service)

  local buffer = {}
  for i, item in D.each(url.searchParams:entries()) do
    local k, v = D.unpack(item)
    buffer[i] = { k, v }
  end
  table.sort(buffer, compare)
  for i = 1, #buffer do
    local item = buffer[i]
    buffer[i] = uri_encode(item[1]) .. "=" .. uri_encode(item[2])
  end
  local canonical_query_string = table.concat(buffer, "&")

  local timestamp = headers:get "x-amz-date"
  if D.is_falsy(timestamp) then
    timestamp = os.date "!%Y%m%dT%H%M%SZ"
    headers:set("x-amz-date", timestamp)
  end
  local date = timestamp:sub(1, 8)
  local hashed_payload = headers:get "x-amz-content-sha256"
  if D.is_falsy(hashed_payload) then
    if body then
      hashed_payload = hex(sha256(await(arrayBuffer(body))))
    else
      hashed_payload = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    end
    headers:set("x-amz-content-sha256", hashed_payload)
  end

  local canonical_headers = {}
  for i, item in D.each(headers:entries()) do
    local k, v = D.unpack(item)
    canonical_headers[i] = { k:lower(), trim(v) }
  end
  if not headers:has "host" then
    canonical_headers[#canonical_headers + 1] = { "host", host }
  end

  table.sort(canonical_headers, compare)

  local buffer = {}
  for i = 1, #canonical_headers do
    buffer[i] = canonical_headers[i][1]
  end
  local signed_headers = table.concat(buffer, ";")

  local buffer = {}
  for i = 1, #canonical_headers do
    local item = canonical_headers[i]
    buffer[i] = item[1] .. ":" .. item[2] .. "\n"
  end
  local canonical_headers = table.concat(buffer)

  local canonical_request =
    method .. "\n" ..
    uri_encode_path(url.pathname) .. "\n" ..
    canonical_query_string .. "\n" ..
    canonical_headers .. "\n" ..
    signed_headers .. "\n" ..
    hashed_payload

  local scope = date .. "/" .. region .. "/" .. service .. "/aws4_request"

  local string_to_sign =
    "AWS4-HMAC-SHA256\n" ..
    timestamp .. "\n" ..
    scope .. "\n" ..
    hex(sha256(canonical_request))

  local signing_key = get_signature_key(secret_key, date, region, service)

  local signature = hmac_sha256(signing_key, string_to_sign)

  local result = "AWS4-HMAC-SHA256 " ..
    "Credential=" .. access_key .. "/" .. scope .. "," ..
    "SignedHeaders=" .. signed_headers .. "," ..
    "Signature=" .. hex(signature)
  headers:set("authorization", result)
  return headers
end

return class
