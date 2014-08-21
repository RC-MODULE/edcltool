local nmc = { }



local function nmc_to_arm(addr)
   if (addr < 0x00040000) then
      addr = addr + 0x00050000;
   end
   addr = addr * 4;
   return addr;
end




nmc.REG_CODEVERSION  = nmc_to_arm(0x100)
nmc.REG_ISR_ON_START = nmc_to_arm(0x101)
nmc.REG_CORE_STATUS  = nmc_to_arm(0x102)
nmc.REG_CORE_START   = nmc_to_arm(0x103)
nmc.REG_PROG_ENTRY   = nmc_to_arm(0x104)
nmc.REG_PROG_RETURN  = nmc_to_arm(0x105)


nmc.STATE_COLD       = 0
nmc.STATE_IDLE       = 1
nmc.STATE_RUNNING    = 2


local function dbg(...)
   if (nmc.debug) then
      print("nmc: ", ...);
   end
end

local function sleep(n) 
   k = os.execute("sleep "..n);
      if (k==nil) then 
	 print("aborted!");
	 os.exit(1); 
      end
end 
			
nmc.dumpregs = function()
   print(string.format("ver    %x ", edcl_read(4, nmc.REG_CODEVERSION)))
   print(string.format("isr    %x ", edcl_read(4, nmc.REG_ISR_ON_START)))
   print(string.format("status %x ", edcl_read(4, nmc.REG_CORE_STATUS)))
   print(string.format("start  %x ", edcl_read(4, nmc.REG_CORE_START)))
   print(string.format("entry  %x ", edcl_read(4, nmc.REG_PROG_ENTRY)))
   print(string.format("return %x ", edcl_read(4, nmc.REG_PROG_RETURN)))
end
	       
nmc.nmi = function()
   edcl_write(4,0x2003c004,0x00000000);  
   edcl_write(4,0x2003c004,0x00000001);  
end

-- Can be safely called only once!
nmc.reset = function()
   edcl_write(4,0x2003c010,0x000000FF);    
end

nmc.get_status  = function()
   local n = edcl_read(4, nmc.REG_CORE_STATUS);
   dbg("status is " .. n);
   return n
end

nmc.wait_status = function(s)
   repeat 
      sleep(0.5);
      n = nmc.get_status()
   until n == s;
end



nmc.upload = function(filename)
   print("Uploading ELF "..filename.." to system memory ");
   local app = { } ;
   local a,entry = edcl_elf_open(filename);
   a = edcl_elf_getsections(a);
   for i,j in ipairs(a) do
      local need = true;
      
      if (j.sh_size == 0) then
	-- need = false
      end

      j.sh_size = j.sh_size
      if (j.name == ".shstrtab") then
	 need = false
      end


      if (j.name == ".memBankMap") then
	 need = false
      end

      if (j.name == ".arguments") then
	 need = true;
	 print("Arguments section at "..string.format("0x%x",j.sh_addr).." size "..j.sh_size);
	 app['args']=nmc_to_arm(j.sh_addr);	 
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
   
   app['entry'] = entry; 
   return entry;
end


nmc.run = function(entry)
   print(string.format("Starting app, entry point 0x%x",entry))
   edcl_write(4, nmc.REG_PROG_ENTRY, entry);
   edcl_write(4, nmc.REG_CORE_START, 0x1);
   nmc.dumpregs();
   nmc.wait_status(nmc.STATE_RUNNING);
end


nmc.init_core = function(initcode)
   dbg("Initializing core" );
   -- Ignore core id for now, only one core in k1879
   -- We can have random crap in mem after reset, clean status

   n = nmc.get_status(); 

   if (n == nmc.STATE_IDLE) then
      dbg("Core already warmed up & idle");
      return n
   end

   if (n == nmc.STATE_RUNNING) then
      print("nmc: core is currently busy, trying to reset (CTRL+C to terminate)");
      nmc.nmi();
      nmc.wait_status(nmc.STATE_IDLE);
      print("nmc: reset done, core ready");
      return nmc.STATE_IDLE
   end

   -- assume core in cold state
   nmc.dumpregs();
   nmc.upload(initcode);
   nmc.reset();
   nmc.dumpregs();
   nmc.nmi();
   nmc.wait_status(nmc.STATE_IDLE);
   dbg("Loaded IPL API version "..string.format("%x",edcl_read(4, nmc.REG_CODEVERSION)));
   nmc.dumpregs();
   return nmc.STATE_IDLE;    
end

return nmc;


