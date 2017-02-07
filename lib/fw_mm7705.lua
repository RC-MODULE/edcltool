--Params are specified to MM7705
local fw = {}

local bootheader_magic_host_image_valid = 0xdeadc0de
local im0 = 0x00040000

fw.run_code = function(file, slavemode)
  if edcl_upload(im0, file) then
    edcl_write(4, im0, bootheader_magic_host_image_valid)
  else
    error("Failed to upload file or no file found!")
  end
end

return fw
