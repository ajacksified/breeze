Request = class('Request')

function Request:initialize(request)
    
    self.url = request.url
    self.body = request.body
    self.method = request.method
    self.headers = request.headers
    
    local contentType = self.headers['content-type']
    self.type = ''
    if contentType then self.type = contentType:sub(contentType:find("/") + 1, -1) end
    
end

