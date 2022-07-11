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

local base16_encoder = {}
for byte = 0x00, 0xFF do
  base16_encoder[byte] = ("%02x"):format(byte)
end

local function hex(data)
  local source = D.new(G.Uint8Array, data)
  local buffer = {}
  for i = 1, source.length do
    buffer[i] = base16_encoder[source[i - 1]]
  end
  return table.concat(buffer)
end

local function sha256(data)
  return await(subtle:digest("SHA-256", array_buffer(data)))
end

local function hmac_sha256(key, data)
  return await(subtle:sign("HMAC", await(subtle:importKey("raw", array_buffer(key), { name = "HMAC", hash = { name = "SHA-256"} }, false, D.array { "sign" })), array_buffer(data)))
end

local function make_canonical_query_string(search_params)
  local canonical_query_string = {}
  for i, item in D.each(search_params:entries()) do
    local k, v = D.unpack(item)
    canonical_query_string[i] = { k, v }
  end
  table.sort(canonical_query_string, compare)

  local canonical_query_string_buffer = {}
  for i = 1, #canonical_query_string do
    local item = canonical_query_string[i]
    canonical_query_string_buffer[i] = uri_encode(item[1]) .. "=" .. uri_encode(item[2])
  end

  return table.concat(canonical_query_string_buffer, "&")
end

local function make_canonical_headers(headers, host, body)
  local date = headers:get "x-amz-date"
  if D.is_falsy(date) then
    date = os.date "!%Y%m%dT%H%M%SZ"
    headers:set("x-amz-date", date)
  end

  local content_sha256 = headers:get "x-amz-content-sha256"
  if D.is_falsy(content_sha256) then
    if body then
      content_sha256 = hex(sha256(await(arrayBuffer(body))))
    else
      content_sha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    end
    headers:set("x-amz-content-sha256", content_sha256)
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

  local canonical_headers_buffer = {}
  for i = 1, #canonical_headers do
    local item = canonical_headers[i]
    canonical_headers_buffer[i] = item[1] .. ":" .. item[2] .. "\n"
  end

  local signed_headers_buffer = {}
  for i = 1, #canonical_headers do
    signed_headers_buffer[i] = canonical_headers[i][1]
  end

  return table.concat(canonical_headers_buffer), table.concat(signed_headers_buffer, ";"), date, content_sha256
end

local function make_signature_key(secret_key, date, region, service)
  return hmac_sha256(hmac_sha256(hmac_sha256(hmac_sha256("AWS4" .. secret_key, date), region), service), "aws4_request")
end

local class = {}

function class.sign(access_key, secret_key, method, url, headers, body)
  local url = D.new(G.URL, url)
  local host = url.host
  local headers = D.new(G.Headers, headers)

  -- https://github.com/boto/botocore/blob/develop/botocore/data/endpoints.json
  local service, region = host:match "([^%.]+)%.(%a%a%-%w+%-%d+)%.amazonaws%.com"
  if not service then
    service = assert(host:match "([^%.]+)%.amazonaws.com")
    region = "us-east-1"
  end

  local canonical_query_string = make_canonical_query_string(url.searchParams)
  local canonical_headers, signed_headers, datetime, hashed_payload = make_canonical_headers(headers, host, body)
  local date = datetime:sub(1, 8)

  local scope = table.concat({
    date;
    region;
    service;
    "aws4_request";
  }, "/")

  local canonical_request = table.concat({
    method;
    uri_encode_path(url.pathname);
    canonical_query_string;
    canonical_headers;
    signed_headers;
    hashed_payload;
  }, "\n")

  local string_to_sign = table.concat({
    "AWS4-HMAC-SHA256";
    datetime;
    scope;
    hex(sha256(canonical_request));
  }, "\n")

  local signature = hex(hmac_sha256(make_signature_key(secret_key, date, region, service), string_to_sign))

  local authorization = ("AWS4-HMAC-SHA256 Credential=%s/%s, SignedHeaders=%s, Signature=%s"):format(access_key, scope, signed_headers, signature)
  headers:set("authorization", authorization)

  return headers
end

return class
