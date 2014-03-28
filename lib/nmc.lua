local nmc = { }


local function nmc_to_arm(addr)
   if (addr < 0x00040000) then
      addr = addr + 0x00050000;
   end
   addr = addr * 4;
   return addr;
end

nmc.upload = function(filename)
   print("Uploading "..filename.." to system memory ");
   a = edcl_elf_open(filename);
   a = edcl_elf_getsections(a);
   for i,j in ipairs(a) do
      local need = true;
      
      if (j.sh_size == 0) then
	 need = false
      end

            
      if (need) then
	 addr = nmc_to_arm(j.sh_addr);
	 print("Uploading section: "..j.name.." "..j.sh_size.." bytes to 0x"..string.format("%x",addr));
	 n = edcl_upload_chunk(addr, filename, j.sh_offset, j.sh_size, 1);
	 print(n)
      else
	 print("Skipping section: "..j.name.." "..j.sh_size.." bytes addr 0x"..string.format("%x",addr));
      end
   end
end


nmc.reset = function()
   print("nmc: reset")
   edcl_write(4,0x2003c010,0x000000FF);    
end

nmc.run = function()
   print("nmc: start")
   edcl_write(4,0x2003c004,0x00000001);
end



return nmc;


