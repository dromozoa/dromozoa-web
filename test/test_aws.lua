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

local future = async(function ()
  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html

  local aws = async.require "dromozoa.web.aws"

  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html#example-signature-calculations
  local access_key = "AKIAIOSFODNN7EXAMPLE"
  local secret_key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY"

  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html#example-signature-GET-object
  local req = D.new(G.Request, "https://examplebucket.s3.amazonaws.com/test.txt", {
    headers = {
      Range = "bytes=0-9";
      ["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
      ["x-amz-date"] = "20130524T000000Z";
    };
  })
  local expect = "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,SignedHeaders=host;range;x-amz-content-sha256;x-amz-date,Signature=f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41"
  local result = aws.sign(access_key, secret_key, req)
  assert(result == expect)

  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html#example-signature-PUT-object
  local req = D.new(G.Request, "https://examplebucket.s3.amazonaws.com/test$file.text", {
    method = "PUT";
    headers = {
      -- Date = "Fri, 24 May 2013 00:00:00 GMT";
      ["x-amz-date"] = "20130524T000000Z";
      ["x-amz-storage-class"] = "REDUCED_REDUNDANCY";
      ["x-amz-content-sha256"] = "44ce7dd67c959e0d3524ffac1771dfbba87d2b6b4b4e99e42034a8b803f8b072";
    };
    body = "Welcome to Amazon S3.";
  })
  local expect = "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,SignedHeaders=date;host;x-amz-content-sha256;x-amz-date;x-amz-storage-class,Signature=98ad721746da40c64f1a55b78f14c238d841ea1380cd77a1b5971af0ece108bd"
  local result = aws.sign(access_key, secret_key, req)
  print(expect)
  print(result)
  -- assert(result == expect)

  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html#example-signature-GET-bucket-lifecycle
  local req = D.new(G.Request, "https://examplebucket.s3.amazonaws.com?lifecycle", {
    headers = {
      ["x-amz-date"] = "20130524T000000Z";
      ["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    };
  })
  local expect = "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,SignedHeaders=host;x-amz-content-sha256;x-amz-date,Signature=fea454ca298b7da1c68078a5d1bdbfbbe0d65c699e0f91ac7a200a0136783543"
  local result = aws.sign(access_key, secret_key, req)
  assert(result == expect)

  -- https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html#example-signature-list-bucket
  local req = D.new(G.Request, "https://examplebucket.s3.amazonaws.com?max-keys=2&prefix=J", {
    headers = {
      ["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
      ["x-amz-date"] = "20130524T000000Z";
    };
  })
  local expect = "AWS4-HMAC-SHA256 Credential=AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request,SignedHeaders=host;x-amz-content-sha256;x-amz-date,Signature=34b48302e7b5fa45bde8084f4b7868a86f0a534bc59db6670ed5711ef69dc6f7"
  local result, headers = aws.sign(access_key, secret_key, req)
  assert(result == expect)

  local req = D.new(G.Request, req, { headers = headers })
  G.console:log(req)
  for i, item in D.each(req.headers:entries()) do
    print(D.unpack(item))
  end
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
