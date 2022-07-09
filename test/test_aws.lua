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
local async, await = require "dromozoa.web.async" .import "await"

local window = D.window
local crypto = window.crypto
local subtle = crypto.subtle

local function to_hex_string(buffer)
  local source = D.new(window.Uint8Array, buffer)
  local result = {}
  for i = 1, source.length do
    result[i] = ("%02x"):format(source[i - 1])
  end
  return table.concat(result)
end

local function hmac_sha256(k, m)
  local key = await(subtle:importKey("raw", k, { name = "HMAC", hash = { name = "SHA-256"} }, false, D.array { "sign" }))
  return await(subtle:sign("HMAC", key, m))
end

local function get_signature_key(key, date, region, service)
  return hmac_sha256(hmac_sha256(hmac_sha256(hmac_sha256(
    D.slice("AWS4" .. key),
    D.slice(date)),
    D.slice(region)),
    D.slice(service)),
    D.slice "aws4_request")
end

local function sha256(m)
  return await(subtle:digest("SHA-256", D.slice(m)))
end

local future = async(function ()
  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html

  local access_key = "AKIAIOSFODNN7EXAMPLE"
  local secret_key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"

  local datetime = "20130524T000000Z"
  local date = "20130524"
  local region = "us-east-1"
  local service = "s3"
  local sha256_empty = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

  local http_method = "GET"
  local canonical_uri = "/test.txt"
  local canonical_query_string = ""
  local headers = {
    Host = "examplebucket.s3.amazonaws.com";
    Range = " bytes=0-9 ";
    ["x-amz-content-sha256"] = sha256_empty;
    ["x-amz-date"] = datetime;
  }
  local canonical_headers = {}
  local signed_headers = {}
  local hashed_payload = sha256_empty

  for k, v in pairs(headers) do
    local k = k:lower()
    local v = v:gsub("^%s+", ""):gsub("%s+$", "")
    canonical_headers[k] = v
    signed_headers[#signed_headers + 1] = k
  end
  table.sort(signed_headers)

  local buffer = {}
  buffer[#buffer + 1] = ("%s\n%s\n%s\n"):format(http_method, canonical_uri, canonical_query_string)
  for i = 1, #signed_headers do
    local k = signed_headers[i]
    local v = canonical_headers[k]
    buffer[#buffer + 1] = ("%s:%s\n"):format(k, v)
  end
  buffer[#buffer + 1] = ("\n%s\n%s"):format(table.concat(signed_headers, ";"), hashed_payload)
  local canonical_request = table.concat(buffer)

  print(canonical_request)
  print(to_hex_string(sha256(canonical_request)))

  local buffer = {}
  buffer[#buffer + 1] = "AWS4-HMAC-SHA256\n"
  buffer[#buffer + 1] = ("%s\n"):format(datetime)
  buffer[#buffer + 1] = ("%s/%s/%s/aws4_request\n"):format(date, region, service)
  buffer[#buffer + 1] = to_hex_string(sha256(canonical_request))
  local string_to_sign = table.concat(buffer)
  print(string_to_sign)

  local key = get_signature_key(secret_key, date, region, service)

  local sig = hmac_sha256(key, D.slice(string_to_sign))
  print(to_hex_string(sig))

  --[[
  local now = os.time()
  local datetime = os.date("!%Y%m%dT%H%M%SZ", now)
  local date = os.date("!%Y%m%d", now)

  local v = get_signature_key("wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY", "20120215", "us-east-1", "iam")
  print(to_hex_string(v))

  print(to_hex_string(sha256 ""))
  ]]

  print "finished"
end)

while true do
  if future then
    if future:is_ready() then
      future:get()
      future = nil
    end
  end
  coroutine.yield()
end
