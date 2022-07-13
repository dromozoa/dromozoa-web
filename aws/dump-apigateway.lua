#! /usr/bin/env lua

local json = require "dromozoa.commons.json"

local function run(command)
  local handle = io.popen(command, "r")
  local content = handle:read "*a"
  assert(handle:close())
  return assert(json.decode(content))
end

local function run_apigatewayv2(command)
  return run("aws apigatewayv2 " .. command)
end

local api_names = {
  "authorizers";
  "deployments";
  "integrations";
  -- "integration-responses";
  "models";
  "routes";
  -- "route-responses";
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

local apis = run_apigatewayv2 "get-apis"
for _, item in ipairs(apis.Items) do
  local api_id = assert(item.ApiId)
  for _, api_name in ipairs(api_names) do
    local command = "get-" .. api_name .. " --api-id " .. api_id
    io.stderr:write("run ", command, "\n")
    local data = run_apigatewayv2(command)
    item[api_name] = data
    local response = responses [api_name]
    if response then
      local response_name = response.name
      for _, item in ipairs(data.Items) do
        local command = "get-" .. response_name .. " --api-id " .. api_id .. " " .. response.opt .. " " .. item[response.ref]
        io.stderr:write("run ", command, "\n")
        item[response_name] = run_apigatewayv2(command)
      end
    end
  end
end

print(json.encode(apis, { pretty = true, stable = true }))
