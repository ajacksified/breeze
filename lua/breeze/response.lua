Response = class('Response')

function Response:initialize(response)
    self._response = response
end

function Response:setStatus(status)
    breezeApi.responseSetStatus(self._response, status)
end

function Response:setBody(body)
    breezeApi.responseSetBody(self._response, body)
end

function Response:setHeader(name, value)
    breezeApi.responseSetHeader(self._response, name, value)
end





