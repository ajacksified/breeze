logging = require "logging"


logger = logging.new(  function(self, level, message)
                        io.stdout:write(logging.prepareLogMsg(nil, os.date(), level, message))
                        return true
                    end
                  )



logger:setLevel (logging.DEBUG)

logger:log(logging.INFO, "sending email")

logger:info("trying to contact server")
logger:warn("server did not responded yet")
logger:error("server unreachable")

-- dump a table in a log message
local tab = { a = 1, b = 2 }
logger:debug(tab)

