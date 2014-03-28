local nmc = { }


local function nmc_to_arm(addr)
   if (addr < 0x00040000) then
      addr = addr + 0x00050000;
   end
   addr = addr * 4;
   return addr;
end

nmc.upload = function(filename)
   print("Uploading ELF "..filename.." to system memory ");
   a,entry = edcl_elf_open(filename);
   a = edcl_elf_getsections(a);
   for i,j in ipairs(a) do
      local need = true;
      
      if (j.sh_size == 0) then
	 need = false
      end

      j.sh_size = j.sh_size
      if (j.name == ".shstrtab") then
	 need = false
      end

      if (j.name == ".memBankMap") then
	 need = false
      end

      addr = nmc_to_arm(j.sh_addr);

     if (j.name == ".bss") then
	 count = j.sh_size
	 print("Cleaning .bss "..count.." bytes");
	 if (count > 0) then 
	    repeat
	       edcl_write(4,addr,0x0);
	       count=count-4;
	    until count == 0;
	 end
     else
	if (need) then
	   print("Uploading section: "..j.name.." "..j.sh_size.." bytes to 0x"..string.format("%x",addr));
	   n = edcl_upload_chunk(addr, filename, j.sh_offset, j.sh_size, 1);
	end
     end
   end
   
   return entry;
end



nmc.TO_NM = nmc_to_arm(0x101);
nmc.FROM_NM = nmc_to_arm(0x100);
nmc.USER_PROGRAM = nmc_to_arm(0x102);
nmc.RET = nmc_to_arm(0x103);


nmc.reset = function()
   edcl_write(4,0x2003c010,0x000000FF);    
end


nmc.start = function()
   edcl_write(4,0x2003c004,0x00000001);
  
end

nmc.get_status  = function()
   return edcl_read(4, nmc.FROM_NM); 
end


nmc.wait_ldr_ready = function()
   repeat 
      k = os.execute("sleep 1");
      if (k~=0) then return(1); end
      n = nmc.get_status()
   until n == 1;
end

nmc.d = function() 
   print(edcl_read(4, nmc.USER_PROGRAM));
   print(edcl_read(4, nmc.FROM_NM));
   print(edcl_read(4, nmc.TO_NM));
   print("----");
end

nmc.run = function(entry)
   print(string.format("Starting app, entry point 0x%x",entry))
   edcl_write(4, nmc.FROM_NM, 0x0);
   edcl_write(4, nmc.USER_PROGRAM, entry);
   edcl_write(4, nmc.TO_NM, 0x2);
end

nmc.load_init_code = function(file)
   print("Loading NMC initial code");
   nmc.upload(file);
   nmc.reset();
   nmc.start();
   nmc.wait_ldr_ready();
   print("NMC loader is up & running running");
end

return nmc;


