Handler = class('Handler')
local urlparse = require 'url'

function Handler:initialize(path)
    self.path = path
end

--- Override to add any logic to be executed before all requests are handled
function Handler:before()
end

--- Override to add any logic to be executed after all requests are handled
function Handler:after()
end

function Handler:onRequest(url)
    
    -- parse request params
    local query = url.query or ""
    
    if request.type == 'x-www-form-urlencoded' then
        query = query .. "&" .. request.body
    end
    
    request.params = urlparse.parse_query(query) or {}

    if next(request.params) then
        breeze.logger:debug("request params")
        breeze.logger:debug(request.params)    
    end  
    
    -- decode request
    if request.type == 'json' then request.body = json.decode(request.body) end

    self:before()
	
	if response.status == nil then
		self:_handle(url)
		self:after()
	else
		-- before() has set status
		breeze.logger:info("not handling the request as before has set status")
	end
	
	self:_send()
end

function Handler:_send()
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

function Handler:_handle(url)
    response.status = 200
    response.type = 'text'
    
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

