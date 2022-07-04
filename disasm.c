#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

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

typedef struct section_struct {
    int name;                    // offset to section name
    int offset;                  // offset to code
    int executable;
    long size;                   // size of section in bytes
    struct section_struct* next; // next section
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

    e->start = f->contents;
    
    e->section_offset = *(long*)(e->start + 0x28);
    short strndx = *(short*)(e->start + 0x3e);
    
    e->section_entry_size = *(short*)(e->start + 0x3a);
    e->section_count = *(short*)(e->start + 0x3c);

    // initialize all sections
    char* curr = e->start + e->section_offset;

    int name = *(int*)curr;
    long offset = *(long*)(curr + 0x18);
    // check progbits and sh_flags
    int executable = (*(int*)(curr + 0x4) == 0x1) && (*(long*)(curr + 0x8) & 0x4);
    long size = *(long*)(curr + 0x20);
    
    e->sections = make_section(name, offset, executable, size);
    section* node = e->sections;

    for(int i = 0; i < e->section_count - 1; i++) {
        curr += e->section_entry_size;

        // set symbol offset
        if (i == strndx - 1)
            e->symbol_offset = *(long*)(curr + 0x18); // sh_offset

        name = *(int*)curr; // first 4 bytes represent name offset
        offset = *(long*)(curr + 0x18); // offset to code
        executable = (*(int*)(curr + 0x4) == 0x1) && (*(long*)(curr + 0x8) & 0x4);
        size = *(long*)(curr + 0x20);

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

void disasm(char* file_name, int output_fd) {
    // do a linear pass through sections
    // disassemble those first (disassemble each section until invalid opcode)
    // look up symbols and strings

    // then break up sections into functions with .symtab

    int fd = open(file_name, O_RDONLY);
    file* f = read_file(fd);
    elf* e = make_elf(f);
    
    // iterate all executable sections
    for (section* curr = e->sections; curr != NULL; curr = curr->next) {
        if (curr->executable) {
            print_section(e, curr);
        }
    }

    write(output_fd, "hello world\n", sizeof("hello world\n"));

    free_elf(e);
    close_file(f);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: <ELF file>\n");
        exit(1);
    }
    
    disasm(argv[1], 0);

    exit(0);
}
