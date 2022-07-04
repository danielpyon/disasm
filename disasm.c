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
    struct section_struct* next; // next section
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

elf* make_elf(file* f) {
    // initialize elf struct from raw bytes
    elf* e = malloc(sizeof(elf));

    e->start = f->contents;
    
    e->section_offset = *(long*)(e->start + 0x28);
    e->symbol_offset = *(short*)(e->start + 0x3e);
    
    e->section_entry_size = *(short*)(e->start + 0x3a);
    e->section_count = *(short*)(e->start + 0x3c);

    // initialize all sections
    char* curr = e->start + e->section_offset;

    int name = *(int*)curr;
    long offset = *(long*)(curr + 0x18);
    e->sections = make_section(name, offset);
    section* node = e->sections;

    for(int i = 0; i < e->section_count - 1; i++) {
        curr += e->section_entry_size;

        name = *(int*)curr; // first 4 bytes represent name offset
        offset = *(long*)(curr + 0x18); // offset to code

        node->next = make_section(name, offset);
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

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: <ELF file>\n");
        exit(1);
    }
    
    int fd = open(argv[1], O_RDONLY);
    
    file* f = read_file(fd);
    elf* e = make_elf(f);

    printf("number of section header table entries : %d\n", e->section_count); // 16
    printf("offset to section header table: %ld\n", e->section_offset);

    free_elf(e);
    close_file(f);

    return 0;
}
