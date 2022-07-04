#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

off_t file_size_bytes(int fd) {
    return lseek(fd, 0, SEEK_END);
}

char* read_file(int fd) {
    off_t nb = file_size_bytes(fd);
    return (char*)mmap(NULL, nb, PROT_READ, MAP_PRIVATE, fd, 0);
}

void close_file(char* file) {
    off_t nb = file_size_bytes(fd);
    munmap(file, nb);
}

typedef struct {
    int name;      // offset to section name
    int offset     // offset to code
    section* next; // next section
} section;

typedef struct {
    char* start;              // pointer to ELF header
    
    long  section_offset;     // section header offset
    short symbol_offset;      // .shstrtab offset

    short section_entry_size; // section header entry size
    short section_count;      // # of section headers

    section* sections;        // pointer to sections
} elf;

char* get_section_name(elf* e, section* s) {
    return NULL;
}

section* make_section(int name, int offset) {
    section* s = malloc(sizeof(section));
    s->name = name;
    s->offset = offset;
    s->next = NULL;
    return s;
}

elf* make_elf(char* file) {
    // initialize elf struct from raw bytes
    elf* e = malloc(sizeof(elf));

    e->start = file;
    
    e->section_offset = *(long*)(file + 0x28);
    e->symbol_offset = *(short*)(file + 0x3e);
    
    e->section_entry_size = *(short*)(file + 0x3a);
    e->section_count = *(short*)(file + 0x3c);

    // initialize all sections
    char* curr = e->start + e->section_offset;

    int name = *(int*)curr;
    long offset = *(long*)(curr + 0x18);
    e->sections = make_section(name, offset);
    section* node = e->sections;

    for(int i = 0; i < e->section_count - 1; i++) {
        curr += e->section_entry_size;

        name = *(int*)curr // first 4 bytes represent name offset
        offset = *(long*)(curr + 0x18) // offset to code

        node->next = make_section(name, offset);
        node = node->next;
    }

    return e;
}

void free_elf(elf* e) {
    
    free(e);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: <ELF file>\n");
        exit(1);
    }
    
    char* elf = read_file(open(argv[1], O_RDONLY));


    char* section_text = get_section(elf + shoff, ".text", e_shentsize, e_shnum);

    printf("number of section header table entries : %d\n", e_shnum);
    printf("offset to section header table: %ld\n", e_shoff);

    close_file(elf);

    return 0;
}
