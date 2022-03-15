#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <elf.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>

#include <filesystem>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <set>

std::set<std::string> libs_glob;
namespace fs = std::filesystem;

std::vector<std::string> get_libs(const std::string &file) {
  std::vector<std::string> ans;
  std::vector<char> file_data = [fname = file]{
      std::ifstream file(fname, std::ios::binary | std::ios::in);
      return std::vector<char> ( std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{} );
  }();

  char *pybytes = file_data.data();
  
  const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
  Elf64_Ehdr elf_hdr;
  memmove(&elf_hdr, pybytes, sizeof(elf_hdr));
  if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
    std::cerr << "Target is not an ELF executable\n";
    return {};
  }
  if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64) {
    std::cerr << "Sorry, only ELF-64 is supported.\n";
    return {};
  }
  if (elf_hdr.e_machine != EM_X86_64) {
    std::cerr << "Sorry, only x86-64 is supported.\n";
    return {};
  }

  // printf("file size: %zd\n", file_data.size());
  // printf("program header offset: %zd\n", elf_hdr.e_phoff);
  // printf("program header num: %d\n", elf_hdr.e_phnum);
  // printf("section header offset: %zd\n", elf_hdr.e_shoff);
  // printf("section header num: %d\n", elf_hdr.e_shnum);
  // printf("section header string table: %d\n", elf_hdr.e_shstrndx);

  size_t string_offset = elf_hdr.e_shstrndx;
  // printf("string offset at %zd\n", string_offset);
  // printf("\n");

  char *cbytes = (char *)pybytes;

  size_t dynstr_off = 0;
  size_t dynsym_off = 0;
  size_t dynsym_sz = 0;
  size_t dynsect_off = 0;
  size_t dynsect_sz = 0;

  for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
    size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;
    Elf64_Shdr shdr;
    memmove(&shdr, pybytes + offset, sizeof(shdr));
    switch (shdr.sh_type) {
      case SHT_SYMTAB:
      case SHT_STRTAB:
        // TODO: have to handle multiple string tables better
        if (!dynstr_off) {
          // printf("found string table at %zd\n", shdr.sh_offset);
          dynstr_off = shdr.sh_offset;
        }
        break;
      case SHT_DYNSYM:
        dynsym_off = shdr.sh_offset;
        dynsym_sz = shdr.sh_size;
        // printf("found dynsym table at %zd, size %zd\n", shdr.sh_offset,
              //  shdr.sh_size);
        break;
      case SHT_DYNAMIC:
        dynsect_off = shdr.sh_offset;
        dynsect_sz = shdr.sh_size;
        break; 
      default:
        break;
    }
  }
  assert(dynstr_off);
  assert(dynsym_off);
  // printf("final value for dynstr_off = %zd\n", dynstr_off);
  // printf("final value for dynsym_off = %zd\n", dynsym_off);
  // printf("final value for dynsym_sz = %zd\n", dynsym_sz);

  for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsym_sz; j++) {
    Elf64_Sym sym;
    size_t absoffset = dynsym_off + j * sizeof(Elf64_Sym);
    memmove(&sym, cbytes + absoffset, sizeof(sym));
    // printf("SYMBOL TABLE ENTRY %zd\n", j);
    // printf("st_name = %d", sym.st_name);
    if (sym.st_name != 0) {
      // printf(" (%s)", cbytes + dynstr_off + sym.st_name);
    }
    // printf("\n");
    // printf("st_info = %d\n", sym.st_info);
    // printf("st_other = %d\n", sym.st_other);
    // printf("st_shndx = %d\n", sym.st_shndx);
    // printf("st_value = %p\n", (void *)sym.st_value);
    // printf("st_size = %zd\n", sym.st_size);
    if (sym.st_shndx == STN_UNDEF) {
        //printf("undefinded symbol\n");
    } 
    else if (ELF64_ST_TYPE(sym.st_info) == STT_FUNC && ELF64_ST_BIND(sym.st_info) == STB_GLOBAL) {
        //printf("global func\n");
    }
    else if (ELF64_ST_TYPE(sym.st_info) == STT_OBJECT && ELF64_ST_BIND(sym.st_info) == STB_GLOBAL) {
        //printf("global obj\n");
    }
    //printf("\n");
  }
  //printf("\n");

  if (!dynsect_off) {
      return {};
  }
  
  //printf("PRINT dependend libs\n");
  size_t dt_strtab_ofs = 0;

  for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsect_sz; j++) {
    Elf64_Dyn dyn;
    size_t absoffset = dynsect_off + j * sizeof(Elf64_Dyn);
    memmove(&dyn, cbytes + absoffset, sizeof(dyn));
    if (dyn.d_tag == DT_STRTAB) {
        dt_strtab_ofs = dyn.d_un.d_val;
    }
  }
  for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsect_sz; j++) {
    Elf64_Dyn dyn;
    size_t absoffset = dynsect_off + j * sizeof(Elf64_Dyn);
    memmove(&dyn, cbytes + absoffset, sizeof(dyn));
    if (dyn.d_tag == DT_NEEDED) {
        //printf("NEEDED %s %lx\n", cbytes + dt_strtab_ofs + dyn.d_un.d_val, dyn.d_un.d_val);
        std::string s = std::string(cbytes + dt_strtab_ofs + dyn.d_un.d_val);
        ans.push_back(s);
    }
  }
  return ans;
}

fs::path find_so_in(const std::string &name, const fs::path &p) {
  if (p == "") return {};
  for (const fs::directory_entry& dir: fs::recursive_directory_iterator(p)){
      fs::path pt(dir.path());
      if (pt.filename() == name) {
        return pt;
      }
  }
  return {};
}

fs::path find_so_ld_library_path(const std::string &name) {
  std::string s = std::getenv("LD_LIBRARY_PATH");
    std::vector<std::string> sts;
    boost::split(sts, s, boost::is_any_of(":"));

    for (auto e: sts) {
      auto p = find_so_in(name, fs::path(e));
      if (!p.empty()) {
        return p;
      }
    }
  return {};
}

fs::path find_so(const std::string &name) {
  auto p = find_so_ld_library_path(name);
  if (!p.empty()) {
    return p;
  }

  p = find_so_in(name, fs::path("/lib"));
  if (!p.empty()) {
    return p;
  }
  p = find_so_in(name, fs::path("/usr/lib"));
  if (!p.empty()) {
    return p;
  }
  return {};
}

void grep_libs(const std::vector<std::string> &libs) {
  for (auto e: libs) {
    if (libs_glob.find(e) != libs_glob.end()) continue;
    libs_glob.insert(e);
    auto p = find_so(e);
    std::cout << p << std::endl;
    auto libs = get_libs(p);
    grep_libs(libs);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: %s <elf-binary>\n", argv[0]);
    return 1;
  }

  auto files = get_libs(argv[1]);

  for (auto lib: files) {
    std::cout << lib << std::endl;
  }

  std::cout << "------------" << std::endl;
  grep_libs(files);
  
  return 0;
}