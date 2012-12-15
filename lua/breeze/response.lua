Response = class('Response')

function Response:initialize(response)
    self._response = response
	self.body = ''
    self.headers = {}
end

function Response:finish()
    self._response.status = self.status
    self._response.body = self.body
    self._response.headers = self.headers
end





