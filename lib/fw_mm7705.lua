--Params are specified to MM7705
fw = {}

local bootheader_magic_host_image_valid = 0xdeadc0de
local im0 = 0x00040000

fw.run_code = function(file, slavemode)
  edcl_upload(im0, "hello-world.img")
  edcl_write(4, imo, bootheader_magic_host_image_valid)
end

return fw
