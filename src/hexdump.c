#define EZ_IMPLEMENTATION
#define EZ_NO_CRT_LIB
#include "ez.h"

void
uint8_to_hex(uint8_t num, char *buff)
{
    size_t len;
    size_t i;
    uint8_t mul;
    uint8_t initial_mul;
    uint8_t tmp;

    len = 2;
    initial_mul = 0x10;
    mul = initial_mul;
    for(i = 0;
        i < len;
        ++i)
    {
        tmp = ((num / mul) % 0x10);
        if(tmp < 0x0a) 
        {
            buff[i] = (char)('0' + tmp);
        }
        else
        {
            buff[i] = (char)('a' + (tmp - 0x0a));
        }
        mul /= 0x10;
    }
    buff[len] = 0;
}

#include <windows.h>

void
uint64_to_hex(uint64_t num, char *buff)
{
    size_t len;
    size_t i;
    uint64_t mul;
    uint64_t initial_mul;
    uint64_t tmp;

    len = 16;
    initial_mul = 0x1000000000000000;
    mul = initial_mul;
    for(i = 0;
        i < len;
        ++i)
    {
        tmp = ((num / mul) % 0x10);
        if(tmp < 0x0a) 
        {
            buff[i] = (char)('0' + tmp);
        }
        else
        {
            buff[i] = (char)('a' + (tmp - 0x0a));
        }
        mul /= 0x10;
    }
    buff[len] = 0;
}

/*
 * Canonical hex format:
 *
 *      offset                        content
 * 0000000000000000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 *
 * additional ascii content
 *   |................|
 *
 */
void
output_canonical_hex(uint8_t *content, size_t content_size)
{
    uint64_t offset;

    size_t line_len;
    size_t padding;

    size_t i;

    char buff[17];

    char ascii_chars[17];

    size_t pos;

    /*
     * format:
     *     16 chars => offset
     *      2 chars => spaces
     * 3*16-1 chars => content
     *      2 char  => space
     *      1 char  => tab
     *     16 chars => ascii content
     *      1 char  => tab
     */
    line_len = 16 + 2 + 3*16-1 + 2 + 1 + 16 + 1;

    offset = 0;
    for(i = 0;
        i < content_size;
        ++i)
    {
        if((i % 0x10) == 0)
        {
            if(i > 0)
            {
                ascii_chars[16] = 0;
                ez_out_print("  |");
                ez_out_print(ascii_chars);
                ez_out_print("|\n");
                offset += 0x10;
            }
            uint64_to_hex(offset, buff);
            ez_out_print(buff);
            ez_out_print("  ");
        }
        else
        {
            ez_out_print(" ");
        }

        uint8_to_hex(content[i], buff);

        if(ez_char_is_print(content[i]))
        {
            ascii_chars[i % 0x10] = (char)content[i];
        }
        else
        {
            ascii_chars[i % 0x10] = '.';
        }

        ez_out_print(buff);
    }

    pos = (i % 0x10 > 0) ? (i % 0x10) : 0x10;
    ascii_chars[pos] = 0;

    padding = line_len - 16 - 2 - 3*(pos) - 16 - 1;
    for(i = 0;
        i < padding;
        ++i)
    {
        ez_out_print(" ");
    }

    ez_out_print("|");
    ez_out_print(ascii_chars);
    ez_out_print("|\n");
}

char **
parse_args(int *argc)
{
    char **argv;

    char *cmd_line;
    int argv_cap;

    char *start;
    int len;

    char *ptr;

    cmd_line = GetCommandLineA();

    argv_cap = 4;
    argv = ez_mem_alloc(sizeof(char *)*argv_cap);

    *argc = 0;
    while(*cmd_line)
    {
        switch(*cmd_line)
        {
            case '"':
            {
                ++cmd_line;
                start = cmd_line;
                len = 0;
                while(*cmd_line && *cmd_line != '"')
                {
                    ++cmd_line;
                    ++len;
                }
                ptr = (char *)ez_mem_alloc(len + 1);
                ez_str_copy_max(start, ptr, len);
                ptr[len] = 0;

                if(*argc >= argv_cap)
                {
                    argv_cap *= 2;
                    argv = ez_mem_realloc(argv, sizeof(char *)*argv_cap);
                }
                argv[*argc] = ptr;
                *argc += 1;
            } break;

            case  ' ': case '\t': case '\v':
            case '\n': case '\r': case '\f':
            {
                ++cmd_line;
            } break;

            default:
            {
                start = cmd_line;
                len = 0;
                while(*cmd_line &&
                      *cmd_line !=  ' ' && *cmd_line != '\t' &&
                      *cmd_line != '\v' && *cmd_line != '\n' &&
                      *cmd_line != '\r' && *cmd_line != '\f')
                {
                    ++cmd_line;
                    ++len;
                }
                ptr = (char *)ez_mem_alloc(len + 1);
                ez_str_copy_max(start, ptr, len);
                ptr[len] = 0;

                if(*argc >= argv_cap)
                {
                    argv_cap *= 2;
                    argv = ez_mem_realloc(argv, sizeof(char *)*argv_cap);
                }
                argv[*argc] = ptr;
                *argc += 1;
            } break;
        }
    }

    return(argv);
}

void
print_usage(void)
{
    ez_out_println("Usage: ./hexdump file");
}

void
main(void)
{
    int argc;
    char **argv;

    int target_stdin = 0;
    char *file_name = 0;
    uint8_t *content = 0;
    size_t content_size = 0;
    size_t content_cap = 0;

    HANDLE stdin_handle;
    DWORD bytes_to_read;
    DWORD bytes_read;

    argv = parse_args(&argc);

    if(argc <= 1)
    {
        /* TODO: You can eventually hexdump the stdin */
        /*target_stdin = 1;*/
        print_usage();
        return;
    }
    else
    {
        target_stdin = 0;
        file_name = argv[1];
    }

    if(target_stdin)
    {
        stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
        if(stdin_handle && stdin_handle != INVALID_HANDLE_VALUE)
        {
            bytes_read = 0;
            bytes_to_read = 0x1000;
            content_cap = 0;
            content_size = 0;
            do
            {
                content_cap += bytes_to_read;
                content = ez_mem_realloc(content, content_cap);
                if(ReadFile(
                    stdin_handle, content + content_size,
                    bytes_to_read, &bytes_read, 0))
                {
                    content_size += bytes_read;
                }
                else
                {
                    break;
                }
            } while(bytes_read > 0);

            if(content && content_size > 0)
            {
                output_canonical_hex(content, content_size);
            }

            ez_mem_free(content);
        }
    }
    else
    {
        if(ez_file_exists(file_name))
        {
            content = (uint8_t *)ez_file_read_bin(file_name, &content_size);
            if(content && content_size > 0)
            {
                output_canonical_hex(content, content_size);
            }
            ez_file_free(content);
        }
        else
        {
            ez_out_print("File \"");
            ez_out_print(file_name);
            ez_out_print("\" not found!");
        }
    }
}
