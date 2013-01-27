require 'breeze_common'
breeze.environment = 'test'

json = require("cjson")

local exports = {}

local function request(req)
    __req = req
    breeze.onRequest()
end

function exports.get(url, headers)
    headers = headers or {}
    request{url=url, body='', headers=headers, method='GET'}
end

function exports.post(url, body, headers)
    headers = headers or {}
    request{url=url, body=body, headers=headers, method='POST'}
end

return exports