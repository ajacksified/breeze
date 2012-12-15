require 'io'

local mimetypes = require 'mimetypes'

StaticHandler = class('StaticHandler')

function StaticHandler:initialize(options)
    self.options = options or {}
end

function StaticHandler:onRequest(url)
    
    local path = self.options.path..url.path
    local file = io.open(path, "rb")

    if not file then 
        response.body = url.path..' not found'
        response.status = 404
    else
        response.body = file:read("*all")
        file:close()
        response.status = 200
        response.headers['Content-Type'] =  mimetypes.guess(path)
    end
        
	
end


