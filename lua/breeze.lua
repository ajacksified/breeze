--- OOP support
require 'middleclass'

-- breeze class definitions
require 'breeze.request'
require 'breeze.response'
require 'breeze.handler'

--- lua std lib
require 'std'

local logging = require 'logging'

local url = require 'url'

breeze = {}
breeze.handlers = {}
breeze.routes = {}

breeze.environment = breezeApi.environment
breeze.logger = logging.new(function(self, level, message)
                            io.stdout:write(logging.prepareLogMsg(nil, os.date(), level, message))
                            return true
                            end
                            )

local function findHandler(url)
        -- get url base path
    local path = url:split('/')[2]
    -- look for exact match
    local handler = breeze.handlers[path]
    
    return handler, path
end

--- Add route.
-- @param route definition as a table {route="GET /hello/:id", handler=HandlerClass action="id"}
-- route is specified as HTTP method combined with relative url. parts of url prefixed with ":" will get passed to action method of handler instance as request paramaters
function breeze.addRoute(definition)
        
    local method, pattern = "", ""
        
    -- break down route string
    for i, w  in ipairs(definition.route:split(' ')) do
        if i == 1 then  method = w else pattern = w end
    end
    
    -- remove trailing slash
    if pattern:sub(-1, -1) == "/" then
        pattern = pattern:sub(1, -2)
    end

    -- get base path
    local path = pattern:split('/')[2]
    
    -- check if there is a handler for this path
    if not breeze.handlers[path] then
        breeze.handlers[path] = definition.handler
    end

    definition.handler.addRoute{method = method, pattern = pattern, action = definition.action}
end

--- Add Handler
-- @param handler definition as a table {path="/hello", handler=HandlerClass}
-- any request to the url containing path specified in handler definition will be passed to instance of handler class. If exists method corresponding to HTTP method is called
function breeze.addHandler(definition)
    breeze.handlers[definition.path] = definition.handler
end

local function onRequest(req, res)
    
    -- set up globals 
    request = Request:new(req)
    response = Response:new(res)
    
    breeze.logger:info("request: %s %s", request.method, request.url)
    
    -- look for handler
    local urlinfo = url.parse(request.url)

    -- look for exact match
    local handler, path = findHandler(urlinfo.path)

    -- not found    
    if not handler then
        response:setStatus(404)
        response:setBody(urlinfo.path .. " not found")
        breeze.logger:info("response: %d", 404)
        return
    end
    
    if not instanceOf(Handler, handler) then
        --create handler instance
        handler = handler:new(path)
        breeze.handlers[path] = handler
    end
    
    handler:onRequest(urlinfo)
    
    breeze.logger:info("response: %d", response.status)
end

breezeApi.setOnRequest(onRequest)
