require 'breeze_common'

-- if we  are in test mode
if not breeze.environment then
    breeze.environment = breezeApi.environment

    breezeApi.setOnRequest(breeze.onRequest)

    -- /metrics
    MetricsHandler = class('MetricsHandler', Handler)
    function MetricsHandler:get()
        response.body = breezeApi.metrics or {}
    end 

    breeze.addHandler{path='/metrics', handler=MetricsHandler}
else
end
