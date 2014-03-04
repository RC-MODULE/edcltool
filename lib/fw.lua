#!/usr/local/bin/edcltool

local fw = { }

-- Params are specific to K1879. 
-- Further chips may change this

local cmdaddr    = 0;
local magic_addr = 0x00100000;
local mbootoffs  = 0x00100010;
local TEXT_BASE  = 0x00100014;
local RUNPTR     = 0x00100010;
local RAM_BASE   = 0x40100000;



local function magic(value)
   edcl_write(4, magic_addr, value);
end

local function wait_magic(value)
   edcl_wait(4, magic_addr, value);
end

local function wait_nmagic(value)
   edcl_nwait(4, magic_addr, value);
end


function fw.mboot_cmd(cmd)
   print("Running: "..cmd);
   edcl_writestring(cmdaddr, cmd);
   magic(0xdeadbeaf);
   wait_nmagic(0xdeadbeaf);
end

local function get_buffer(addr) 
   edcl_nwait(4, addr, 0xa1); -- wait til slave posts a buffer   
   return edcl_read(4, addr); -- get it
end

local function shift_read(n,addr) 
   v=edcl_read(n, addr);
   addr=addr+n;
   return v,addr;
end

function fw.flash_part(part, file, withoob)
   result=0;

   fsize = edcl_filesize(file);

   if (fsize == -1) then
      print("Fatal: can't access image file");
      return -1
   end

   fw.mboot_cmd("eupgrade "..part); 
   es = edcl_read(4, magic_addr);
   erase,es = shift_read(4, es);
   oob,es = shift_read(4, es);
   psize,es = shift_read(8, es);
   wsize = shift_read(4, es);
   
   print("mtd: partition "..part.." erasesize "..erase.." oob "..oob);
   print("mtd: size "..psize.." writesize "..wsize);

   if withoob then 
      oob_per_eraseblock = (erase / writesize) * oob
      chunksize = erase + oob_per_eraseblock;
      imagemaxsize = psize + psize/erase * oob_per_eraseblock;
   else
      chunksize = erase;
      imagemaxsize=psize; 
   end

   if fsize > imagemaxsize then
      print("Fatal: Image file too big for partition")
      print(fsize.." > "..imagemaxsize);
      return -1;
   end

   print("chunksize " .. chunksize);
   offset = 0;

   magic(0xa1);

   while true do
      buffer = get_buffer(magic_addr);
      ret = edcl_upload_chunk(buffer, file, offset, chunksize);
      if (ret == -1) then
	 print("FATAL: Cannot access file: ", file);
	 ret = 0; -- fallthrough to finish it
	 result= -1; -- we failed
      end
      if (ret == 0) then
	 edcl_write(4, magic_addr, 0x0); -- signal that we're done with it
	 edcl_nwait(4, magic_addr, 0x0); -- wait for command state
	 return result;
      end
      edcl_write(4, magic_addr, 0xa1);
      offset = offset + chunksize;   
   end
end


function fw.run_code(file, slavemode)
   edcl_upload(mbootoffs, file);
   magic(0x0); -- in case something left after soft-reset
   print("Starting code...")
   if (slavemode) then
      print("Slave mode enabled");
      magic(0xdeadc0de);
   end
   edcl_write(4, RUNPTR, TEXT_BASE);
   if (slavemode) then
      print('Waiting for board to respond');
      edcl_nwait(4,0x00100000,0xdeadc0de);
      cmdaddr = edcl_read(4,0x00100000);
      print('Command handle obtained!');
   end
end

function fw.mboot_cmd_list(list)
   for j,k in ipairs(list) do
      fw.mboot_cmd(k); 
   end
end

local mboot_burn_cmds = {
   "mtd scrub 0x0 0x80000;",
   "mtd write 0x40100000 0x0 0x40000",
}

function fw.write_bootloader(file)
   edcl_upload(RAM_BASE, file);
   fw.mboot_cmd_list(mboot_burn_cmds);
   print("Bootloader burned & ready")
end

function fw.partition(tbl)
   local parts="setenv parts "
   local env = { } 
   for j,k in ipairs(tbl) do
      parts=parts..k[1]..",";
      if (type(k[2])=='number') then
	 table.insert(env,"setenv "..k[1].."_size "..string.format("0x%x",k[2]))
      else
	 table.insert(env,"setenv "..k[1].."_size -")
      end
   end
   fw.mboot_cmd(parts);
   fw.mboot_cmd_list(env);
   fw.mboot_cmd("partscan");
   fw.mboot_cmd("saveenv");
end
 
return fw