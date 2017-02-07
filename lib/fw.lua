local chip_name = edcl_init()

print(chip_name)

local fw = {
  ["K1879XB"] = require("fw_k1879"),
  ["MM7705MPW"] = require("fw_mm7705"),
}

local ret = fw[chip_name]

if ret == nil then
  error("Unknow chip")
end

return ret
