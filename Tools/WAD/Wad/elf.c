/* ----------------------------------------------------------------------------- 
 * elf.c
 *
 *     ELF file management. This file contains functions for accessing ELF
 *     file data from a raw memory mapped ELF file (as performed by the
 *     functions in object.c).
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

#include "wad.h"
#ifdef WAD_SOLARIS
#include <sys/elf.h>
#endif
#ifdef WAD_LINUX
#include <elf.h>
#endif


/* --- What's needed here (high level interface) :

     - Mapping of addresses to symbols
     - Mapping of symbols to file+line
     - Parameter passing information?

*/


/* -----------------------------------------------------------------------------
 * wad_elf_check()
 *
 * Checks to see if an object file is an ELF file. Returns 1 on success and 
 * changes the type flag of wo to indicate the type of ELF file.
 * ----------------------------------------------------------------------------- */

int
wad_elf_check(WadObjectFile *wo) {
  int t;

  if (strncmp((char *)wo->ptr,ELFMAG, SELFMAG) != 0)
    return 0;
  
  /* Probably need to put some kind of 32/64 bit check here */
  return 1;
}

/* -----------------------------------------------------------------------------
 * wad_elf_phdrcnt()
 * 
 * Return number of entries in the ELF program header section
 * ----------------------------------------------------------------------------- */

int
wad_elf_phdrcnt(WadObjectFile *wo) {
  Elf32_Ehdr *eh;
  
  eh = (Elf32_Ehdr *) wo->ptr;
  return eh->e_phnum;
}

/* -----------------------------------------------------------------------------
 * wad_elf_phdrpos()
 *
 * Return the location of the ELF program header.
 * ----------------------------------------------------------------------------- */

void *
wad_elf_phdrpos(WadObjectFile *wo) {
  Elf32_Ehdr *eh;
  char *c;
  eh = (Elf32_Ehdr *) wo->ptr;
  c = (char *) wo->ptr;
  return (void *) (c+eh->e_phoff);
}

/* -----------------------------------------------------------------------------
 * wad_elf_shdrcnt()
 * 
 * Return number of entries in the ELF section header
 * ----------------------------------------------------------------------------- */

int
wad_elf_shdrcnt(WadObjectFile *wo) {
  Elf32_Ehdr *eh;
  
  eh = (Elf32_Ehdr *) wo->ptr;
  return eh->e_shnum;
}


/* -----------------------------------------------------------------------------
 * wad_elf_shdrpos()
 *
 * Return the location of the section headers
 * ----------------------------------------------------------------------------- */

void *
wad_elf_shdrpos(WadObjectFile *wo) {
  Elf32_Ehdr *eh;
  char *c;
  eh = (Elf32_Ehdr *) wo->ptr;
  c = (char *) wo->ptr;
  return (void *) (c+eh->e_shoff);
}

/* -----------------------------------------------------------------------------
 * wad_elf_section_header()
 *
 * Get a specific section number
 * ----------------------------------------------------------------------------- */

void *wad_elf_section_header(WadObjectFile *wo, int sn) {
  Elf32_Ehdr *eh;
  char  *r;

  eh = (Elf32_Ehdr *) wo->ptr;
  if ((sn < 0) || (sn >= eh->e_shnum)) return 0;
  
  r = (char *) wad_elf_shdrpos(wo) + (sn*eh->e_shentsize);
  return (void *) r;
}

/* -----------------------------------------------------------------------------
 * wad_elf_section_data()
 *
 * Get section data
 * ----------------------------------------------------------------------------- */

void *wad_elf_section_data(WadObjectFile *wo, int sn) {
  Elf32_Shdr *sh;
  char *r;

  sh = (Elf32_Shdr *) wad_elf_section_header(wo,sn);
  if (!sh) return 0;
  
  r = ((char *) wo->ptr) + sh->sh_offset;
  return r;
}

/* -----------------------------------------------------------------------------
 * wad_elf_section_size()
 * Return section size
 * ----------------------------------------------------------------------------- */

int wad_elf_section_size(WadObjectFile *wo, int sn) {
  Elf32_Shdr *sh;

  sh = (Elf32_Shdr *) wad_elf_section_header(wo,sn);
  if (!sh) return -1;
  return sh->sh_size;
}

/* -----------------------------------------------------------------------------
 * wad_elf_section_name()
 *
 * Returns the name of an ELF section 
 * ----------------------------------------------------------------------------- */

char *wad_elf_section_name(WadObjectFile *wo, int sn) {
  Elf32_Ehdr *eh;
  Elf32_Shdr *sh;
  char    *sectionstr;

  eh = (Elf32_Ehdr *) wo->ptr;

  /* Get the string table */
  sectionstr = (char *) wad_elf_section_data(wo,eh->e_shstrndx);
  if (!sectionstr) {
    return 0;
  }

  /* Get the section header for the section */
  sh = (Elf32_Shdr *) wad_elf_section_header(wo,sn);
  if (!sh) return 0;
  return sectionstr + sh->sh_name;
}

/* -----------------------------------------------------------------------------
 * wad_elf_section_byname()
 *
 * Get section data given a section name
 * ----------------------------------------------------------------------------- */

int
wad_elf_section_byname(WadObjectFile *wo, char *name) {
  int i;
  char *sn;
  int n;

  n = wad_elf_shdrcnt(wo);
  for (i = 0; i <n; i++) {
    sn = wad_elf_section_name(wo,i);
    if (strcmp(sn,name) == 0) {
      return i;
    }
  }
  return -1;
}

/* -----------------------------------------------------------------------------
 * wad_find_symbol()
 *
 * Tries to find the symbol associated with a given address. base is the
 * base address of the object file.
 * ----------------------------------------------------------------------------- */


char *
wad_elf_find_symbol(WadObjectFile *wo, void *ptr, unsigned long base, WadSymbol *ws) {
  int nsymtab;
  int nstrtab;
  int symtab_size;
  Elf32_Sym  *sym;
  int         nsym;
  char       *str;
  int i;
  unsigned long vaddr;
  char *name;
  char *localfile = 0;

  vaddr = (unsigned long) ptr;

  nsymtab = wad_elf_section_byname(wo,".symtab");
  if (nsymtab < 0) goto dynsym;
  nstrtab = wad_elf_section_byname(wo,".strtab");
  if (nstrtab < 0) goto dynsym;
  
  symtab_size = wad_elf_section_size(wo,nsymtab);
  sym = (Elf32_Sym *) wad_elf_section_data(wo,nsymtab);
  str = (char *) wad_elf_section_data(wo,nstrtab);

  nsym = (symtab_size/sizeof(Elf32_Sym));
  for (i = 0; i < nsym; i++) {
    name = str + sym[i].st_name;
    /* Look for filename in case the symbol maps to a local symbol */
    if (ELF32_ST_TYPE(sym[i].st_info) == STT_FILE) {
      localfile = name;
    }
    if (wad_debug_mode & DEBUG_SYMBOL_SEARCH) {
      wad_printf("%x(%x): %s   %x + %x, %x, %x\n", base, vaddr, name, sym[i].st_value, sym[i].st_size, sym[i].st_info, sym[i].st_shndx);
    }

    if (((base + sym[i].st_value) <= vaddr) && (vaddr <= (base+sym[i].st_value + sym[i].st_size))) {
#ifdef WAD_LINUX
      /* If the section index is 0, the symbol is undefined */
      if (sym[i].st_shndx == 0) continue;
#endif
      ws->value = sym[i].st_value;
      if (ELF32_ST_BIND(sym[i].st_info) == STB_LOCAL) {
	ws->file = localfile;
	ws->bind = WS_LOCAL;
      } else {
	ws->bind = WS_GLOBAL;
      }
      ws->name = name;
      return name;
    }
  }

 dynsym:

  /* If we didn't find it in the .symtab section. Maybe it's in the .dynsym, .dynstr section */
  
  nsymtab = wad_elf_section_byname(wo,".dynsym");
  if (nsymtab < 0) return 0;
  nstrtab = wad_elf_section_byname(wo,".dynstr");
  if (nstrtab < 0) return 0;

  symtab_size = wad_elf_section_size(wo,nsymtab);
  sym = (Elf32_Sym *) wad_elf_section_data(wo,nsymtab);
  str = (char *) wad_elf_section_data(wo,nstrtab);

  nsym = (symtab_size/sizeof(Elf32_Sym));
  localfile = 0;
  for (i = 0; i < nsym; i++) {
    name = str + sym[i].st_name;
    if (ELF32_ST_TYPE(sym[i].st_info) == STT_FILE) {
      localfile = name;
    }
    if (wad_debug_mode & DEBUG_SYMBOL_SEARCH) {
      wad_printf("%x(%x): %s   %x + %x, %x, %x\n", base, vaddr, name, sym[i].st_value, sym[i].st_size, sym[i].st_info, sym[i].st_shndx);
    }
    if (((base + sym[i].st_value) <= vaddr) && (vaddr <= (base+sym[i].st_value + sym[i].st_size))) {
#ifdef WAD_LINUX
      /* If the section index is 0, the symbol is undefined */
      if (sym[i].st_shndx == 0) continue;
#endif
      ws->value = sym[i].st_value;
      if (ELF32_ST_BIND(sym[i].st_info) == STB_LOCAL) {
	ws->file = localfile;
	ws->bind = WS_LOCAL;
      } else {
	ws->bind = WS_GLOBAL;
      }
      ws->name = name;
      return name;
    }
  }
  return 0;
}

/* -----------------------------------------------------------------------------
 * wad_elf_debug_info()
 *
 * Gather debugging information about a function (if possible)
 * ----------------------------------------------------------------------------- */

int
wad_elf_debug_info(WadObjectFile *wo, WadSymbol *wsym, unsigned long offset, WadDebug *wd) {
  int nstab, nstabstr, nstabindex, nstabindexstr, nstabexcl, nstabexclstr;
  int ret;
  void *stab;
  char *stabstr;
  int   stabsize;

  nstab = wad_elf_section_byname(wo,".stab");
  nstabstr = wad_elf_section_byname(wo,".stabstr");
  nstabindex = wad_elf_section_byname(wo,".stab.index");
  nstabindexstr = wad_elf_section_byname(wo,".stab.indexstr");
  nstabexcl = wad_elf_section_byname(wo,".stab.excl");
  nstabexclstr = wad_elf_section_byname(wo,".stab.exclstr");

#ifdef DEBUG_DEBUG
  wad_printf("nstab         = %d\n", nstab);
  wad_printf("nstabstr      = %d\n", nstabstr);
  wad_printf("nstabindex    = %d\n", nstabindex);
  wad_printf("nstabindexstr = %d\n", nstabindexstr);
  wad_printf("nstabexcl     = %d\n", nstabexcl);
  wad_printf("nstabexclstr  = %d\n", nstabexclstr);
#endif 

  /* Now start searching stabs */

  /* Look in the .stab section */
  if (nstab > 0) {
    stab = wad_elf_section_data(wo,nstab);
    stabsize = wad_elf_section_size(wo,nstab);
    stabstr = (char *) wad_elf_section_data(wo,nstabstr);
    
    if (wad_search_stab(stab,stabsize,stabstr, wsym, offset,wd)) return 1;
  }

  /* Look in the .stab.excl section. A solaris oddity? */
  
  if (nstabexcl > 0) {
    stab = wad_elf_section_data(wo,nstabexcl);
    stabsize = wad_elf_section_size(wo, nstabexcl);
    stabstr = (char *) wad_elf_section_data(wo, nstabexclstr);
    if (wad_search_stab(stab,stabsize,stabstr, wsym, offset,wd)) return 1;
  }

  /* Look in the .stab.index section. A Solaris oddity? */
  if (nstabindex > 0) {
    stab = wad_elf_section_data(wo,nstabindex);
    stabsize = wad_elf_section_size(wo, nstabindex);
    stabstr = (char *) wad_elf_section_data(wo, nstabindexstr);
    if (wad_search_stab(stab,stabsize,stabstr, wsym, offset, wd)) {
      WadObjectFile *wo1;
      /* Hmmm. Might be in a different file */
      char objfile[MAX_PATH];
      strcpy(objfile, wd->objfile);
      wo1 = wad_object_load(objfile);
      if (wo1) {
	ret = wad_debug_info(wo1,wsym,offset,wd);
      } else {
	/*	wad_printf("couldn't load %s\n", objfile); */
      }
      if (!ret) return wad_search_stab(stab,stabsize,stabstr,wsym, offset,wd);
      return ret;
    }
  }
  return 0;
}

/* -----------------------------------------------------------------------------
 * wad_elf_debug()
 *
 * Print some debugging information about an object
 * ----------------------------------------------------------------------------- */

void
wad_elf_debug(WadObjectFile *wo) {
  int i;
  wad_printf("ELF Debug : obj = %x (%s)\n", wo, wo->path);
  wad_printf("   phdrcnt      = %d\n", wad_elf_phdrcnt(wo));
  wad_printf("   phdrpos      = %x\n", wad_elf_phdrpos(wo));
  wad_printf("   shdrcnt      = %d\n", wad_elf_shdrcnt(wo));
  wad_printf("   shdrpos      = %x\n", wad_elf_shdrpos(wo));
  for (i = 0; i < wad_elf_shdrcnt(wo); i++) {
    wad_printf("      section '%s': data = 0x%x, size = %d\n", 
	   wad_elf_section_name(wo,i),
	   wad_elf_section_data(wo,i),
	   wad_elf_section_size(wo,i));
  }
}

/* general purpose functions exposed to the outside world */

/* -----------------------------------------------------------------------------
 * wad_find_symbol()
 * ----------------------------------------------------------------------------- */
 
char *
wad_find_symbol(WadObjectFile *wo, void *ptr, unsigned base, WadSymbol *ws) {
  char *r;
  ws->name = 0;
  ws->file = 0;
  ws->type = 0;
  ws->bind = 0;
  ws->value = 0;
  if (wad_debug_mode & DEBUG_SYMBOL) {
    wad_printf("wad: Searching for 0x%08x --> ", ptr);
  }
  r = wad_elf_find_symbol(wo,ptr,base,ws);
  if (r) {
    if (wad_debug_mode & DEBUG_SYMBOL) {
      wad_printf("%s", ws->name);
      if (ws->file)
	wad_printf(" in '%s'\n", ws->file);
      else
	wad_printf("\n");
    }
  } else {
    if (wad_debug_mode & DEBUG_SYMBOL) {
      wad_printf("?\n");
    }
  }
  return r;
}

int
wad_debug_info(WadObjectFile *wo, WadSymbol *wsym, unsigned long offset, WadDebug *wd) {
  return wad_elf_debug_info(wo,wsym, offset,wd);
}
