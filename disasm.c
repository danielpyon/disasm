#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

// points to start of section header
#define E_SHOFF 0x28
// index of section header entry with section names
#define E_SHSTRNDX 0x3e
// contains size of a section header entry
#define E_SHENTSIZE 0x3a
// contains number of entries in section header table
#define E_SHNUM 0x3c

// offset of section
#define SH_OFFSET 0x18
// type of header
#define SH_TYPE 0x4
// program data
#define SHT_PROGBITS 0x1 
// attributes of the section
#define SH_FLAGS 0x8
// executable section
#define SHF_EXECINSTR 0x4
// size of section in bytes
#define SH_SIZE 0x20

#define MAX_INSTRUCTION_LENGTH 15

off_t file_size_bytes(int fd) {
    return lseek(fd, 0, SEEK_END);
}

// represents a file
typedef struct {
    int size_bytes;
    char* contents;
} file;

file* read_file(int fd) {
    // read from fd into file struct
    file* f = malloc(sizeof(file));
    f->size_bytes = file_size_bytes(fd);
    f->contents = (char*)mmap(NULL, f->size_bytes, PROT_READ, MAP_PRIVATE, fd, 0);

    return f;
}

void close_file(file* f) {
    munmap(f, f->size_bytes);
    free(f);
}

typedef struct section {
    int name;                    // offset to section name
    int offset;                  // offset to code
    int executable;
    long size;                   // size of section in bytes
    struct section* next; // next section
} section;

typedef struct {
    char* start;              // pointer to ELF header
    
    long  section_offset;     // section header offset
    long  symbol_offset;      // .shstrtab offset

    short section_entry_size; // section header entry size
    short section_count;      // # of section headers

    section* sections;        // pointer to sections
} elf;

char* get_section_name(elf* e, section* s) {
    int offset = s->name;
    char* name = e->start + e->symbol_offset + offset;
    return name;
}

section* get_section(elf* e, char* name) {
    section* curr;
    for (curr = e->sections; curr != NULL; curr = curr->next)
        if (!strcmp(get_section_name(e, curr), name))
            return curr;
    return NULL;
}

char* get_section_code(elf* e, section* s) {
    return e->start + s->offset;
}

section* make_section(int name, int offset, int executable, long size) {
    section* s = malloc(sizeof(section));
    s->name = name;
    s->offset = offset;
    s->executable = executable;
    s->size = size;
    s->next = NULL;
    return s;
}

elf* make_elf(file* f) {
    // initialize elf struct from raw bytes
    elf* e = malloc(sizeof(elf));

    // set start pointer to the beginning of the file
    e->start = f->contents;
    
    e->section_offset = *(long*) (e->start + E_SHOFF);
    short strndx      = *(short*)(e->start + E_SHSTRNDX);
    
    e->section_entry_size = *(short*)(e->start + E_SHENTSIZE);
    e->section_count      = *(short*)(e->start + E_SHNUM);

    // initialize all sections
    char* curr = e->start + e->section_offset;

    int name    = *(int*)  curr;
    long offset = *(long*)(curr + SH_OFFSET);
    int executable = *(int*)(curr + SH_TYPE) == SHT_PROGBITS
        && *(long*)(curr + SH_FLAGS) & SHF_EXECINSTR;
    long size = *(long*)(curr + SH_SIZE);
    
    e->sections = make_section(name, offset, executable, size);
    section* node = e->sections;

    for(int i = 0; i < e->section_count - 1; i++) {
        curr += e->section_entry_size;

        // set symbol offset
        if (i == strndx - 1)
            e->symbol_offset = *(long*)(curr + SH_OFFSET);

        name    = *(int*)  curr;
        offset = *(long*)(curr + SH_OFFSET);
        executable = *(int*)(curr + SH_TYPE) == SHT_PROGBITS
            && *(long*)(curr + SH_FLAGS) & SHF_EXECINSTR;
        size = *(long*)(curr + SH_SIZE);

        node->next = make_section(name, offset, executable, size);
        node = node->next;
    }

    return e;
}

void free_elf(elf* e) {
    section* prev = NULL;
    section* curr;

    for (curr = e->sections; curr != NULL; curr = curr->next) {
        free(prev);
        prev = curr;
    }
    free(prev);
    free(e);
}

void print_elf(elf* e) {
    int width = 20;
    printf("ELF information:\n");

    printf("- ELF header start:      %20p (address)\n", e->start);

    printf("- Section Header Offset: %20ld bytes\n", e->section_offset);
    printf("- .shstrtab Offset:      %20ld bytes\n", e->symbol_offset);

    printf("- Section Entry Size:    %20d bytes\n", e->section_entry_size);
    printf("- Section Entry Count:   %20d items\n", e->section_count);
}

void print_section(elf* e, section* s) {
    int width = 20;
    printf("Section information:\n");

    printf("- Name:   %20s\n", get_section_name(e, s));
    printf("- Offset: %20d bytes\n", s->offset);
    printf("- Size:   %20ld bytes\n", s->size);
}

char* g_prefix_table[256];
#define P_LOCK 0xf0
#define P_REPNE_REPNZ 0xf2
#define P_REP_REPE_REPZ 0xf3
#define P_CS 0x2e
#define P_SS 0x36
#define P_DS 0x3e
#define P_ES 0x26
#define P_FS 0x64
#define P_GS 0x65
#define P_BRANCH_NOT_TAKEN 0x2e
#define P_BRANCH_TAKEN 0x3e
#define P_DEFAULT_OPERAND_SIZE_OVERRIDE 0x66
#define P_DEFAULT_ADDRESS_SIZE_OVERRIDE 0x67

#define REX(x) !((x >> 4) ^ 0b0100)
#define REX_W 0b100 
#define REX_R 0b100
#define REX_X 0b10
#define REX_B 0b1

const char endbr64[4] = { 0xf3, 0x0f, 0x1e, 0xfa };

void init_prefix_table() {
    memset(g_prefix_table, 0, sizeof(g_prefix_table));
    g_prefix_table[P_LOCK] = "lock";
    g_prefix_table[P_REPNE_REPNZ] = "repne/repnz";
    g_prefix_table[P_REP_REPE_REPZ] = "rep/repe/repz";
    g_prefix_table[P_CS] = "";
    g_prefix_table[P_SS] = "";
    g_prefix_table[P_DS] = "";
    g_prefix_table[P_ES] = "";
    g_prefix_table[P_FS] = "";
    g_prefix_table[P_GS] = "";
    g_prefix_table[P_BRANCH_NOT_TAKEN] = "";
    g_prefix_table[P_BRANCH_TAKEN] = "";
    g_prefix_table[P_DEFAULT_OPERAND_SIZE_OVERRIDE] = "";
    g_prefix_table[P_DEFAULT_ADDRESS_SIZE_OVERRIDE] = "";
}

// returns the number of bytes in the instruction
int disassemble_x86(char* code, int ip, int output_fd) {
    char inst[MAX_INSTRUCTION_LENGTH];
    memcpy(inst, code + ip, sizeof(inst));

    int sz = 0;
    
    if (!strncmp(inst, endbr64, sizeof(endbr64)))
        return sizeof(endbr64);

    // check the prefix bytes
    if (g_prefix_table[inst[sz]]) {
        char* prefix = g_prefix_table[inst[sz++]];
        while (prefix) {
            prefix = g_prefix_table[inst[sz++]];
            dprintf(output_fd, "%s ", prefix);
        }
    }

    char rex = inst[sz];
    if (REX(rex))
        sz++;
    
    // opcode
    char opcode;
    if (inst[sz] == 0x0f) {
        sz++;
        if (inst[sz] == 0x38 || inst[sz] == 0x3a)
            sz++;
    }
    opcode = inst[sz++];

    dprintf(output_fd, "opcode: %hhx ", inst[sz]);
    write(output_fd, "\n", sizeof("\n"));

    

    return sz;
}

void disasm(int input_fd, int output_fd) {
    // do a linear pass through sections
    // disassemble those first (disassemble each section until invalid opcode)
    // look up symbols and strings

    // then break up sections into functions with .symtab
    file* f = read_file(input_fd);
    elf* e = make_elf(f);
    
    // iterate all executable sections
    for (section* curr = e->sections; curr != NULL; curr = curr->next) {
        if (curr->executable) {
            char* code = get_section_code(e, curr);
            long len = curr->size;

            dprintf(output_fd, "Disassembly of section <%s>:\n", get_section_name(e, curr));

            if (strcmp(get_section_name(e, curr), ".text"))
                continue;

            disassemble_x86(code, 4, output_fd);

            /*
            int ip = 0;
            while(ip < len) {
                ip += disassemble_x86(code, ip, output_fd);
            }
            */

            break;
        }
    }

    free_elf(e);
    close_file(f);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: <ELF file>\n");
        exit(1);
    }
    
    init_prefix_table();

    int input_fd = open(argv[1], O_RDONLY);
    disasm(input_fd, 0);

    exit(0);
}
