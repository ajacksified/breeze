--- OOP support
require 'middleclass'

-- breeze class definitions
require 'breeze.request'
require 'breeze.response'
require 'breeze.handler'
require 'breeze.static_handler'

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
    pattern:rtrim("/")

    local path = pattern
    
    -- get base path
    if pattern:find("/:") > 1 then
       -- get base path
           path = pattern:sub(1, pattern:find("/:") - 1)
       end
    
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
    breeze.handlers[definition.path] = {handler=definition.handler, options=definition.options}
end

local function onRequest(req, res)

    -- set up globals 
    __res = {}
    request = Request:new(__req)
    response = Response:new(__res)
    
    breeze.logger:info("request: %s %s", request.method, request.url)
    
    -- look for handler
    local urlinfo = url.parse(request.url)
    
    local path = urlinfo.path
    -- get url base path

    local definition = breeze.handlers[path]
    
    -- look for closest match
    if not definition then
        local parts = list.reverse(request.url:split('/'))
        for i, part in ipairs(parts) do 
            path = path:sub(1, #path - #part)
            
            if #path > 1 then path = path:rtrim("/") end 
            definition = breeze.handlers[path]
            if definition then break end
        end
    end
    
    -- not found    
    if not definition then
        response.status = 404
        response.body = urlinfo.path .. " not found"
    else
        local handler = definition.handler
    
        if not instanceOf(Handler, handler) then
            --create handler instance
            handler = handler:new(definition.options)
            definition.handler = handler
        end

        -- handle request
        handler:onRequest(urlinfo)            
    end
    
    breeze.logger:info("response: %d", response.status)
    response:finish()
end

breezeApi.setOnRequest(onRequest)

-- /metrics
MetricsHandler = class('MetricsHandler', Handler)
function MetricsHandler:get()
    response.body = breezeApi.metrics or {}
end 

breeze.addHandler{path='/metrics', handler=MetricsHandler}
