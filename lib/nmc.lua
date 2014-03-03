local nmc = { }

nmc.upload = function(offset, filename)
   print("Uploading "..filename.." @ phys offset "..string.format("0x%x",offset));
   a = edcl_elf_open(filename);
   a = edcl_elf_getsections(a);
   for i,j in ipairs(a) do
      local need = true;
      
      if (j.sh_size == 0) then
	 need = false
      end
      
      if (j.sh_addr == 0) then
	 need = false
      end
      
      if (need) then
	 addr = offset + j.sh_addr;
	 print("Uploading section: "..j.name.." "..j.sh_size.." bytes to 0x"..string.format("%x",addr));
	 edcl_upload_chunk(addr, filename, j.sh_offset, j.sh_size, 1);
      else
	 print("Skipping section: "..j.name.." "..j.sh_size.." bytes addr "..j.sh_addr);
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


