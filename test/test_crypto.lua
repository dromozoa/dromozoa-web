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

local function import_key(secret)
  return subtle:importKey("raw", secret, { name = "HMAC", hash = { name = "SHA-256" } }, false, D.array { "sign" })
end

local future = async(function (self)
  print(crypto:randomUUID())

  local buffer = self:await(subtle:digest("SHA-256", D.slice "foobarbaz"))
  local digest = to_hex_string(buffer)
  assert(digest == "97df3588b5a3f24babc3851b372f0ba71a9dcdded43b14b9d06961bfc1707d9d")

  local buffer = self:await(subtle:digest("SHA-256", D.slice "日本語\0あいうえお"))
  local digest = to_hex_string(buffer)
  assert(digest == "43b0fa97739f6c418d9643ae9488cfd0180037bf7a04c59bba33617069d38067")

  -- local key = self:await(subtle:generateKey({ name = "HMAC", hash = { name = "SHA-256" } }, true, D.array { "sign", "verify" }))
  -- local jwk = self:await(subtle:exportKey("jwk", key))
  -- print(window.JSON:stringify(jwk))

  -- https://docs.aws.amazon.com/general/latest/gr/signature-v4-examples.html
  local secret = D.slice("AWS4" .. "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY")
  assert(to_hex_string(secret) == "41575334774a616c725855746e46454d492f4b374d44454e472b62507852666943594558414d504c454b4559")
  local key = self:await(import_key(secret))
  local date = self:await(subtle:sign("HMAC", key, D.slice "20120215"))
  assert(to_hex_string(date) == "969fbb94feb542b71ede6f87fe4d5fa29c789342b0f407474670f0c2489e0a0d")
  local key = self:await(import_key(date))
  local region = self:await(subtle:sign("HMAC", key, D.slice "us-east-1"))
  assert(to_hex_string(region) == "69daa0209cd9c5ff5c8ced464a696fd4252e981430b10e3d3fd8e2f197d7a70c")
  local key = self:await(import_key(region))
  local service = self:await(subtle:sign("HMAC", key, D.slice "iam"))
  assert(to_hex_string(service) == "f72cfd46f26bc4643f06a11eabb6c0ba18780c19a8da0c31ace671265e3c87fa")
  local key = self:await(import_key(service))
  local signing = self:await(subtle:sign("HMAC", key, D.slice "aws4_request"))
  assert(to_hex_string(signing) == "f4780e2d9f65fa895f9c67b32ce1baf0b0d8a43505a000a1a9e090d414db404d")

  print "finished"
end)

while true do
  if future and future:is_ready() then
    future:get()
    future = nil
  end

  assert(D.get_error_queue())
  coroutine.yield()
end
