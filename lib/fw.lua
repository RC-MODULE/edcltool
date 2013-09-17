#!/usr/local/bin/edcltool

module("fw", package.seeall) 

-- Params are specific to K1879. 
-- Further chips may change this

local cmdaddr    = 0;
local magic_addr = 0x00100000;
local mbootoffs  = 0x00100010;
local TEXT_BASE  = 0x00100014;
local RUNPTR     = 0x00100010;
local RAM_BASE   = 0x40100000;



function magic(value)
   edcl_write(4, magic_addr, value);
end

function wait_magic(value)
   edcl_wait(4, magic_addr, value);
end

function wait_nmagic(value)
   edcl_nwait(4, magic_addr, value);
end


function mboot_cmd(cmd)
   print("Running: "..cmd);
   edcl_writestring(cmdaddr, cmd);
   magic(0xdeadbeaf);
   wait_nmagic(0xdeadbeaf);
end

local function get_buffer(addr) 
   edcl_nwait(4, addr, 0xa1); -- wait til slave posts a buffer   
   return edcl_read(4, addr); -- get it
end


function flash_part(part, file, withoob)
   mboot_cmd("eupgrade "..part); 
   es = edcl_read(4, magic_addr);
   erase = edcl_read(4, es);
   oob = edcl_read(4, es + 4);
   print("mtd: "..part.." erasesize "..erase.." oob "..oob);
   -- TODO: This logic sucks, and fails with oob stuff
   if withoob then 
      chunksize = erase + oob; 
   else
      chunksize = erase;
   end
   print("chunksize " .. chunksize);
   offset = 0;

   magic(0xa1);

   while true do
      buffer = get_buffer(magic_addr);
      ret = edcl_upload_chunk(buffer, file, offset, chunksize);
      if (ret == 0) then
	 edcl_write(4, magic_addr, 0x0); -- signal that we're done with it
	 edcl_nwait(4, magic_addr, 0x0); -- wait for command state
	 return;
      end
      edcl_write(4, magic_addr, 0xa1);
      offset = offset + chunksize;   
   end
end


function run_code(file, slavemode)
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

function mboot_cmd_list(list)
   for j,k in ipairs(list) do
      mboot_cmd(k); 
   end
end

local mboot_burn_cmds = {
   "mtd scrub 0x0 0x80000;",
   "mtd write 0x40100000 0x0 0x40000",
}

function write_bootloader(file)
   edcl_upload(RAM_BASE, file);
   mboot_cmd_list(mboot_burn_cmds);
   print("Bootloader burned & ready")
end

function partition(tbl)
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
   mboot_cmd(parts);
   mboot_cmd_list(env);
   mboot_cmd("partscan");
   mboot_cmd("saveenv");
end
 

print(" == EDCL Firmware Library 1.0 ==");