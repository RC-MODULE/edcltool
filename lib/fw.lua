chip = edcl_init()

print(chip)

local fw = {}

if chip == "K1879XB"  then
  fw = require "fw_k1879"
elseif chip == "MM7705MPW" then
  fw = require "fw_mm7705"
else
  error("Unknow chip!\n")
end
