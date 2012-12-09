Handler = class('Handler')
local urlparse = require 'url'

function Handler:initialize(options)
    self.options = options or {}
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
    
    -- add more content types here
    
	response:setBody(response.body) 
end

function Handler:_handle(url)
    response.status = 200
    response.type = 'text'
    
    local match, slashes = url.path:gsub("/", "/")
    
    local methodHandler = nil
    
    -- handle url pattern
    for index, route in ipairs(Handler.routes) do 
        if slashes == route.slashes and request.method == route.method then
            
            local pathTokens = url.path:split("/")
            local patternTokens = route.pattern:split("/")
                        
            for i, token in ipairs(patternTokens) do
                if token:find(":") then request.params[token:ltrim(":")] = pathTokens[i] end
            end
            
            methodHandler = self[route.action]

            if type(methodHandler) == 'function' then
                break
            end
        end
    end
    
    if not table.empty(request.params) then
        breeze.logger:debug("request params " .. tostring(request.params))
    end  
    
  -- handle
    local methodHandler = methodHandler or self[request.method:lower()]
    
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
    
    table.insert(Handler.static.routes, route)
end

Handler.static.routes = {}

