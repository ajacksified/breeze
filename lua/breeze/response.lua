Response = class('Response')

function Response:initialize(response)
    self._response = response
	self.body = ''
end

function Response:setStatus(status)
    assert(type(self) == 'table')
    breezeApi.responseSetStatus(self._response, status)
end

function Response:setBody(body)
    assert(type(self) == 'table')
    breezeApi.responseSetBody(self._response, body)
end

function Response:setHeader(name, value)
    assert(type(self) == 'table')
    breezeApi.responseSetHeader(self._response, name, value)
end





