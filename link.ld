OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(start)

SECTIONS
{
  . = 0xC0100000;

  .text : AT(ADDR(.text) - 0xC0000000) {
    code = .; _code = .; __code = .;
    *(.text)
    *(.rdata)
     . = ALIGN(4096); 
  }

  endCode = .;

  .data  : {
    data = .; _data = .; __data = .;
    *(.data)
     . = ALIGN(4096);
  }

  endData = .;

  symtable_start = .; _symtable_start = .;
  .symtable : {
	*(.symtable)
  }

  symtable_end = .; _symtable_end = .;

  symstrings_start = .; _symstrings_start = .;
  .symstrings : {
	*(.symstrings)
	. = ALIGN(4096);
  }

  symstrings_end = .; _symstrings_end = .;

  .bss  : {
   bss = .; _bss = .; __bss = .;
	*(.bss.page_aligned)
    *(.bss)
	*(COMMON)
	. = ALIGN(4);
  	endbss = .; _endbss = .;
  }

  . = ALIGN(4096);

  initcode_start = .;
  .initcode : {
    *(.initcode)
    . = ALIGN(4096);
    initcode_end = .;
  }

  initdata_start = .;
  .initdata : {
	*(.initdata)
	. = ALIGN(4096);
	initdata_end = .;
  }

  end = .; _end = .; __end = .;
}
