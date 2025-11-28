#include <stdint.h>
#include <stdbool.h>
#include "header/ext2.h"
#include "header/stdlib/string.h"

#define MAX_BUFFER 256 // Kurangi buffer untuk avoid overflow
#define MAX_ARGS 8
#define MAX_PATH_LEN 128

#define FG_WHITE 0xF
#define FG_GREEN 0xA
#define FG_RED 0xC
#define FG_CYAN 0xB
#define FG_YELLOW 0xE

char read_buf[BLOCK_SIZE * 2]; // Kurangi ukuran buffer
char input_buffer[MAX_BUFFER];
char *argv[MAX_ARGS];
int argc = 0;
uint32_t current_inode = 2;
uint32_t parent_inode = 2; // Track parent for cd .. support
char current_path[MAX_PATH_LEN] = "/";
struct EXT2DirectoryEntry cached_entry; // Cache entry to avoid buffer overwrite issues

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx)
{
    __asm__ volatile("mov %0, %%ebx" : : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : : "r"(eax));
    __asm__ volatile("int $0x30");
}

void puts(const char *str, uint8_t color)
{
    syscall(6, (uint32_t)str, strlen(str), color);
}

char getchar()
{
    char c = 0;
    while (c == 0)
    {
        syscall(4, (uint32_t)&c, 0, 0);
    }
    return c;
}

void putchar(char c, uint8_t color)
{
    syscall(5, (uint32_t)c, color, 0);
}

void clear_screen()
{
    syscall(8, 0, 0, 0);
}

void exit()
{
    syscall(10, 0, 0, 0);
}

int8_t read_file(struct EXT2DriverRequest *req)
{
    int8_t retcode;
    syscall(0, (uint32_t)req, (uint32_t)&retcode, 0);
    return retcode;
}

int8_t read_dir(struct EXT2DriverRequest *req)
{
    int8_t retcode;
    syscall(1, (uint32_t)req, (uint32_t)&retcode, 0);
    return retcode;
}

int8_t write_file(struct EXT2DriverRequest *req)
{
    int8_t retcode;
    syscall(2, (uint32_t)req, (uint32_t)&retcode, 0);
    return retcode;
}

int8_t delete_file(struct EXT2DriverRequest *req)
{
    int8_t retcode;
    syscall(3, (uint32_t)req, (uint32_t)&retcode, 0);
    return retcode;
}

void read_line()
{
    memset(input_buffer, 0, MAX_BUFFER);
    int index = 0;
    char c;

    while (index < MAX_BUFFER - 1)
    {
        c = getchar();

        if (c == '\b')
        {
            if (index > 0)
            {
                index--;
                input_buffer[index] = '\0';
                putchar('\b', FG_WHITE);
            }
        }
        else if (c == '\n')
        {
            input_buffer[index] = '\0';
            putchar('\n', FG_WHITE);
            return;
        }
        else if (c >= 32 && c <= 126)
        {
            input_buffer[index] = c;
            putchar(c, FG_WHITE);
            index++;
        }
    }
}

void parse_command()
{
    argc = 0;
    memset(argv, 0, sizeof(argv));

    char *token = input_buffer;
    char *next_token = input_buffer;

    while (*next_token && argc < MAX_ARGS - 1)
    {
        while (*token == ' ')
            token++;
        if (*token == '\0')
            break;

        argv[argc++] = token;

        next_token = token;
        while (*next_token && *next_token != ' ')
            next_token++;

        if (*next_token == ' ')
        {
            *next_token = '\0';
            next_token++;
        }
        token = next_token;
    }
}

int8_t find_entry_in_dir(uint32_t dir_inode, const char *name)
{
    struct EXT2DriverRequest req = {
        .buf = read_buf,
        .parent_inode = dir_inode,
        .buffer_size = sizeof(read_buf) // Gunakan sizeof() agar aman
    };

    int8_t ret = read_dir(&req);
    if (ret != 0)
    {
        // Tidak perlu cetak error di sini, biarkan pemanggil yang menangani
        return 0;
    }

    struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)read_buf;
    uint32_t offset = 0;
    uint8_t name_len = strlen(name);

    // PERBAIKI INI: Loop harus beriterasi di seluruh buffer yang dikembalikan
    while (offset < req.buffer_size)
    {
        entry = (struct EXT2DirectoryEntry *)((uint8_t *)read_buf + offset);

        if (entry->rec_len == 0)
            break;

        if (entry->inode != 0)
        {
            char *entry_name = (char *)entry + 8;

            // Try to match
            bool match = (entry->name_len == name_len);
            if (match)
            {
                for (uint8_t i = 0; i < name_len; i++)
                {
                    if (entry_name[i] != name[i])
                    {
                        match = false;
                        break;
                    }
                }
            }

            if (match)
            {
                cached_entry.inode = entry->inode;
                cached_entry.rec_len = entry->rec_len;
                cached_entry.name_len = entry->name_len;
                cached_entry.file_type = entry->file_type;
                return 1;
            }
        }

        offset += entry->rec_len;
    }
    return 0;
}

int main(void)
{
    syscall(7, 0, 0, 0);
    clear_screen();

    puts("================================================================\n", FG_CYAN);
    puts("  OS-ICIBOS Shell v1.0\n", FG_GREEN);
    puts("  Commands: ls, cat, mkdir, rm, cd, clear, exit\n", FG_WHITE);
    puts("================================================================\n\n", FG_CYAN);

    while (true)
    {
        puts(current_path, FG_CYAN);
        puts("$ ", FG_GREEN);

        read_line();
        parse_command();

        if (argc == 0)
            continue;

        if (strcmp(argv[0], "help") == 0)
        {
            puts("Available commands:\n", FG_YELLOW);
            puts("  ls, cd, cat, mkdir, rm, clear, exit\n", FG_WHITE);
        }
        else if (strcmp(argv[0], "clear") == 0)
        {
            clear_screen();
        }
        else if (strcmp(argv[0], "exit") == 0)
        {
            exit();
        }
        else if (strcmp(argv[0], "ls") == 0)
        {
            struct EXT2DriverRequest req = {
                .buf = read_buf,
                .parent_inode = current_inode,
                .buffer_size = sizeof(read_buf)};

            int8_t ret = read_dir(&req);
            if (ret != 0)
            {
                puts("Error: Cannot read directory\n", FG_RED);
            }
            else
            {
                struct EXT2DirectoryEntry *entry = (struct EXT2DirectoryEntry *)read_buf;
                uint32_t offset = 0;

                while (offset < req.buffer_size && entry->rec_len > 0)
                {
                    char *name = (char *)entry + 8;
                    for (int i = 0; i < entry->name_len; i++)
                        putchar(name[i], FG_WHITE);

                    if (entry->file_type == EXT2_FT_DIR)
                        puts("/", FG_CYAN);
                    puts("  ", FG_WHITE);

                    offset += entry->rec_len;
                    entry = (struct EXT2DirectoryEntry *)((uint8_t *)read_buf + offset);
                }
                puts("\n", FG_WHITE);
            }
        }
        else if (strcmp(argv[0], "cd") == 0)
        {
            if (argc < 2)
            {
                current_inode = 2;
                parent_inode = 2;
                strcpy(current_path, "/");
            }
            else if (strcmp(argv[1], "..") == 0)
            {
                // Go back to parent
                if (current_inode != 2)
                {
                    int8_t found = find_entry_in_dir(parent_inode, "..");
                    if (found)
                        parent_inode = cached_entry.inode;
                    else
                        parent_inode = 2;
                    current_inode = parent_inode;

                    if (strlen(current_path) > 1)
                    {
                        int i = strlen(current_path) - 2;
                        while (i > 0 && current_path[i] != '/')
                            i--;
                        current_path[i + 1] = '\0';
                    }
                }
            }
            else if (strcmp(argv[1], ".") == 0)
            {
                // Stay in current directory
            }
            else
            {
                int8_t found = find_entry_in_dir(current_inode, argv[1]);

                if (found == 0)
                {
                    puts("Error: Directory not found\n", FG_RED);
                }
                else if (cached_entry.file_type != EXT2_FT_DIR)
                {
                    puts("Error: Not a directory (type=", FG_RED);
                    putchar('0' + cached_entry.file_type, FG_RED);
                    puts(")\n", FG_RED);
                }
                else
                {
                    parent_inode = current_inode;
                    current_inode = cached_entry.inode;

                    if (current_path[strlen(current_path) - 1] != '/')
                    {
                        current_path[strlen(current_path)] = '/';
                        current_path[strlen(current_path) + 1] = '\0';
                    }

                    int path_len = strlen(current_path);
                    int name_len = strlen(argv[1]);
                    for (int i = 0; i < name_len; i++)
                    {
                        current_path[path_len + i] = argv[1][i];
                    }
                    current_path[path_len + name_len] = '\0';
                }
            }
        }
        else if (strcmp(argv[0], "cat") == 0)
        {
            if (argc < 2)
            {
                puts("Usage: cat <filename>\n", FG_RED);
            }
            else
            {
                struct EXT2DriverRequest req = {
                    .buf = read_buf,
                    .name = argv[1],
                    .name_len = strlen(argv[1]),
                    .parent_inode = current_inode,
                    .buffer_size = BLOCK_SIZE * 8};

                int8_t ret = read_file(&req);
                if (ret == 0)
                {
                    for (uint32_t i = 0; i < req.buffer_size; i++)
                        putchar(read_buf[i], FG_WHITE);
                    puts("\n", FG_WHITE);
                }
                else
                {
                    puts("Error: File not found\n", FG_RED);
                }
            }
        }
        else if (strcmp(argv[0], "mkdir") == 0)
        {
            if (argc < 2)
            {
                puts("Usage: mkdir <dirname>\n", FG_RED);
            }
            else
            {
                struct EXT2DriverRequest req = {
                    .buf = (void *)0, // mkdir tidak perlu buffer
                    .name = argv[1],
                    .name_len = strlen(argv[1]),
                    .parent_inode = current_inode,
                    .buffer_size = 0,
                    .is_directory = true};

                int8_t ret = write_file(&req);

                if (ret == 0)
                {
                    puts("Directory created successfully\n", FG_GREEN);
                }
                else if (ret == 1)
                {
                    puts("Error: Directory already exists\n", FG_RED);
                }
                else if (ret == 2)
                {
                    puts("Error: Invalid parent directory\n", FG_RED);
                }
                else
                {
                    puts("Error: Failed to create directory\n", FG_RED);
                }
            }
        }
        else if (strcmp(argv[0], "rm") == 0)
        {
            if (argc < 2)
            {
                puts("Usage: rm <filename>\n", FG_RED);
            }
            else
            {
                int8_t found = find_entry_in_dir(current_inode, argv[1]);

                if (found == 0)
                {
                    puts("Error: File not found\n", FG_RED);
                }
                else
                {
                    struct EXT2DriverRequest req = {
                        .buf = (void *)0,
                        .name = argv[1],
                        .name_len = strlen(argv[1]),
                        .parent_inode = current_inode,
                        .buffer_size = 0,
                        .is_directory = (cached_entry.file_type == EXT2_FT_DIR)};

                    int8_t ret = delete_file(&req);

                    if (ret == 0)
                    {
                        puts("File deleted successfully\n", FG_GREEN);
                    }
                    else if (ret == 1)
                    {
                        puts("Error: File not found\n", FG_RED);
                    }
                    else if (ret == 2)
                    {
                        puts("Error: Directory not empty\n", FG_RED);
                    }
                    else
                    {
                        puts("Error: Cannot delete file\n", FG_RED);
                    }
                }
            }
        }
        else
        {
            char msg1[] = "Sorry banget PIPS Command '";
            char msg2[] = "' belum ada nih\n";

            for (int i = 0; msg1[i] != '\0'; i++)
                putchar(msg1[i], FG_RED);

            for (int i = 0; argv[0][i] != '\0'; i++)
                putchar(argv[0][i], FG_RED);

            for (int i = 0; msg2[i] != '\0'; i++)
                putchar(msg2[i], FG_RED);
        }
    }

    return 0;
}