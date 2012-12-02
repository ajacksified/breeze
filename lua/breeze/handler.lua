Handler = class('Handler')
local urlparse = require 'url'

function Handler:initialize()
end

function Handler:before()
end

function Handler:after()
    response:setStatus(response.status)

    if type(response.body) == 'table' then
        -- encode table to json
        response.body = json.encode(response.body)
        response:setHeader('Content-Type', 'application/json')
    elseif response.type == 'text' then 
        response:setHeader('Content-Type', 'text/plain') 
    end
    
    response:setBody(response.body) 
end

function Handler:handle(url)
    self:before()
    self:_handle(url)
    self:after()
end

function Handler:_handle(url)
    response.status = 200
    response.body = ''
    response.type = 'text'
    
    -- parse request params
    local query = url.query or ""
    
    breeze.logger:debug(request.type)
    
    if request.type == 'x-www-form-urlencoded' then
        query = query .. "&" .. request.body
    end
    
    request.params = urlparse.parse_query(query) or {}
    
        
    breeze.logger:debug("request params")
    breeze.logger:debug(request.params)
    
    -- decode request
    if request.type == 'json' then request.body = json.decode(request.body) end
    
    local match, slashes = url.path:gsub("/", "/")
    
    -- handle url pattern
    for index, route in ipairs(Handler.routes) do 
        if slashes == route.slashes and request.method == route.method then
            
            local start, current, startrel, endrel, match = 0, 0, 0, 0, 0

            local rel = url.path:gsub(self.path .. "/", "")
            
            while true do
                start, current, match = route.pattern:find(":(%w+)", start + 1)
                if start == nil then break end
                
                if endrel == 0 then
                    startrel = start  
                else
                    startrel = endrel + 1
                end
                
                endrel = rel:find("/", endrel + 1)
                
                local value = rel:sub(startrel, endrel)
                if value:sub(-1, -1) == "/" then
                    value = value:sub(1, -2)
                end
                
                request.params[match] = value
            end            
            
            
            local methodHandler = self[route.action]

            if type(methodHandler) == 'function' then
                methodHandler(self)
                return
            end
        end
    end
    
  -- handle all 
    local methodHandler = self[request.method:lower()]

    if type(methodHandler) == 'function' then 
        methodHandler(self) 
    else
        breeze.logger:warn("could not find handler for %s %s", request.method, request.url)
        response.status = 501
        response.body = 'Not implemented'
    end
end

function Handler.static.addRoute(route)

    local match, slashes = route.pattern:gsub("/", "/")
    route.slashes = slashes
    route.pattern = route.pattern:sub(route.pattern:find("/:") + 1)
    
    table.insert(Handler.static.routes, route)
end

Handler.static.routes = {}

