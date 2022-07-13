#! /usr/bin/env lua

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

local json = require "dromozoa.commons.json"

local function run(command)
  io.stderr:write(command, "\n")
  local handle = io.popen(command, "r")
  local content = handle:read "*a"
  assert(handle:close())
  return assert(json.decode(content))
end

local function run_apigateway(command)
  return run("aws apigateway " .. command)
end

local function run_apigatewayv2(command)
  return run("aws apigatewayv2 " .. command)
end

local result = {}

local names = {
  "authorizers";
  "deployments";
  "documentation-parts";
  "documentation-versions";
  "gateway-responses";
  "models";
  "request-validators";
  "resources";
  "stages";
}

local root = run_apigateway "get-rest-apis"
for _, item in ipairs(root.items) do
  local rest_api_id = assert(item.id)
  for _, name in ipairs(names) do
    local data = run_apigateway("get-" .. name .. " --rest-api-id " .. rest_api_id)
    item[name] = data
    if name == "models" then
      for _, model in ipairs(data.items) do
        local arg = " --rest-api-id " .. rest_api_id .. " --model-name " .. model.name
        model.model_template = run_apigateway("get-model-template" .. arg)
      end
    elseif name == "resources" then
      for _, resource in ipairs(data.items) do
        local resource_id = resource.id
        for http_method, method in pairs(resource.resourceMethods or {}) do
          local arg = " --rest-api-id " .. rest_api_id .. " --resource-id " .. resource_id .. " --http-method " .. http_method
          method.integration = run_apigateway("get-integration" .. arg)
          -- get-integration-responseは不要
        end
      end
    end
  end
end
result.v1 = root

local names = {
  "authorizers";
  "deployments";
  "integrations";
  "models";
  "routes";
  "stages";
}

local responses = {
  integrations = {
    opt = "--integration-id";
    ref = "IntegrationId";
    name = "integration-responses";
  };
  routes = {
    opt = "--route-id";
    ref = "RouteId";
    name = "route-responses";
  };
}

local root = run_apigatewayv2 "get-apis"
for _, item in ipairs(root.Items) do
  local api_id = assert(item.ApiId)
  for _, name in ipairs(names) do
    local data = run_apigatewayv2("get-" .. name .. " --api-id " .. api_id)
    item[name] = data
    local response = responses[name]
    if response then
      local opt = response.opt
      local ref = response.ref
      local name = response.name
      for _, item in ipairs(data.Items) do
        item[name] = run_apigatewayv2("get-" .. name .. " --api-id " .. api_id .. " " .. opt .. " " .. item[ref])
      end
    end
  end
end
result.v2 = root

print(json.encode(result, { pretty = true, stable = true }))
