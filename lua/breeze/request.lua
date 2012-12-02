Request = class('Request')

function Request:initialize(request)
    self._request = request
    self.url = self:_getUrl()
    self.body = self:_getBody()
    self.method = self:_getMethod()
end

function Request:_getUrl()
    return breezeApi.requestGetUrl(self._request)
end

function Request:_getBody()
    return breezeApi.requestGetBody(self._request)
end

function Request:_getMethod()
    return breezeApi.requestGetMethod(self._request)
end


function Request:header(name)
    assert(type(self) == 'table')
    return breezeApi.requestGetHeader(self._request, name)
end
