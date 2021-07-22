#ifndef EZ_H
#define EZ_H

#ifndef EZ_SCOPE
#define EZ_SCOPE extern
#endif

/******************************************************************************/
/**                                  TYPES                                   **/
/******************************************************************************/

#include <inttypes.h>
#include <stddef.h>

/******************************************************************************/
/**                                  GENERAL                                 **/
/******************************************************************************/

#define ez_min(x, y) (((x)<(y))?(x):(y))
#define ez_max(x, y) (((x)>(y))?(x):(y))

#define ez_array_count(a) ((sizeof(a))/(sizeof(a[0])))

#ifndef NDEBUG
#define ez_assert(cond)\
    if(!(cond))\
    {\
        ez_abort();\
    }
#else
#define ez_assert(cond) ((void *)0)
#endif

void ez_abort(void);

/******************************************************************************/
/**                                  MEMORY                                  **/
/******************************************************************************/

EZ_SCOPE void  ez_mem_copy(void *src, void *dest, size_t bytes);
EZ_SCOPE void *ez_mem_alloc(size_t size);
EZ_SCOPE void  ez_mem_free(void *ptr);
EZ_SCOPE void *ez_mem_realloc(void *ptr, size_t size);

/******************************************************************************/
/**                                 STRINGS                                  **/
/******************************************************************************/

#define ez_char_is_digit(c) ((c) >= '0' && (c) <= '9')
#define ez_char_is_xdigit(c)\
    (ez_char_is_digit(c) ||\
     ((c) >= 'A' && (c) <= 'F') ||\
     ((c) >= 'a' && (c) <= 'f'))
#define ez_char_is_alpha(c) \
    (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define ez_char_is_alphanum(c) (ez_char_is_alpha(c) || ez_char_is_digit(c))
#define ez_char_is_alnum(c) ez_char_is_alphanum(c)
#define ez_char_is_control(c) (((c) >= 0 && (c) <= 31) || (c) == 127)
#define ez_char_is_cntrl(c) ez_char_is_control(c)
#define ez_char_is_lower(c) ((c) >= 'a' && (c) <= 'z')
#define ez_char_is_upper(c) ((c) >= 'A' && (c) <= 'Z')
#define ez_char_is_print(c) ((c) >= 32 && (c) <= 126)
#define ez_char_is_punct(c)\
    (((c) >= 33  && (c) <= 47) ||\
     ((c) >= 58  && (c) <= 64) ||\
     ((c) >= 91  && (c) <= 96) ||\
     ((c) >= 123 && (c) <= 126))
#define ez_char_is_space(c)\
    ((c) ==  ' ' || (c) == '\t' || (c) == '\v' ||\
     (c) == '\n' || (c) == '\r' || (c) == '\f')
#define ez_char_is_graph(c) (ez_char_is_alphanum(c) || ez_char_is_punct(c))

#define ez_char_to_lower(c) ((ez_char_is_upper(c)) ? ((c) + 32) : (c))
#define ez_char_to_upper(c) ((ez_char_is_lower(c)) ? ((c) - 32) : (c))

EZ_SCOPE size_t ez_str_len(char *s);
EZ_SCOPE size_t ez_str_len_max(char *s, size_t max);
EZ_SCOPE void   ez_str_copy(char *src, char *dest);
EZ_SCOPE void   ez_str_copy_max(char *src, char *dest, size_t max);
EZ_SCOPE int    ez_str_decimal(char *s);

/******************************************************************************/
/**                              STD I/O & ERR                               **/
/******************************************************************************/

EZ_SCOPE void ez_out_print(char *s);
EZ_SCOPE void ez_out_println(char *s);

/******************************************************************************/
/**                                 FILE I/O                                 **/
/******************************************************************************/

EZ_SCOPE int    ez_file_exists(char *pathname);
EZ_SCOPE size_t ez_file_size(char *pathname);
EZ_SCOPE char  *ez_file_read_text(char *pathname, size_t *size);
EZ_SCOPE void  *ez_file_read_bin(char *pathname, size_t *size);
EZ_SCOPE void   ez_file_free(void *file_content);
EZ_SCOPE int    ez_file_write(char *pathname, void *content, size_t size);
EZ_SCOPE int    ez_file_append(char *pathname, void *content, size_t size);

#define ez_file_read ez_file_read_text

#endif


/******************************************************************************/
/**                              IMPLEMENTATION                              **/
/******************************************************************************/

#ifdef EZ_IMPLEMENTATION

#ifndef EZ_NO_CRT_LIB

/* TODO: C std lib imlementation */
#error "Not implemented yet with C standard library"

#elif defined(WIN32) || defined(_WIN32) ||\
      defined(__WIN32__) || defined(__NT__)

/*
 * TODO: Should we put this here or not? Let the user solve these problems?
 * Stuff to make MSVC linker not complaining about the absence of
 * the C runtime library
 **/
int _fltused = 0;

/*
 * The only library we need is kernel32.dll (or static kernel32.lib)
 */
#pragma comment(lib, "kernel32.lib")

#include <windows.h>

void
ez_abort(void)
{
    ExitProcess(1);
}

void
ez_mem_copy(void *src, void *dest, size_t bytes)
{
    unsigned int i;

    char *srcp  = (char *)src;
    char *destp = (char *)dest;

    for(i = 0;
        i < bytes;
        ++i)
    {
        *destp++ = *srcp++;
    }
}

typedef struct
{
    size_t size;
} ez_allochdr;

#define ez_get_allochdr(ptr)\
    ((ez_allochdr *)((char *)ptr - sizeof(ez_allochdr)))

void *
ez_mem_alloc(size_t size)
{
    ez_allochdr *hdr;
    void *ptr = 0;
    hdr = (ez_allochdr *)VirtualAlloc(
        0, size+sizeof(ez_allochdr),
        MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if(hdr)
    {
        hdr->size = size;
        ptr = (void *)((char *)hdr + sizeof(ez_allochdr));
    }
    return(ptr);
}

void
ez_mem_free(void *ptr)
{
    void *real_ptr;
    if(ptr)
    {
        real_ptr = ez_get_allochdr(ptr);
        VirtualFree(real_ptr, 0, MEM_RELEASE|MEM_DECOMMIT);
    }
}

void *
ez_mem_realloc(void *ptr, size_t size)
{
    void *newptr = 0;
    ez_allochdr *oldhdr;
    size_t min_size;
    if(ptr)
    {
        if(size == 0)
        {
            ez_mem_free(ptr);
        }
        else
        {
            newptr = ez_mem_alloc(size);
            oldhdr = ez_get_allochdr(ptr);
            min_size = ez_min(size, oldhdr->size);
            if(ptr != newptr)
            {
                ez_mem_copy(ptr, newptr, min_size);
            }
        }
    }
    else
    {
        newptr = ez_mem_alloc(size);
    }
    return(newptr);
}

size_t
ez_str_len(char *s)
{
    size_t len = 0;
    while(*s++)
    {
        ++len;
    }
    return(len);
}

size_t
ez_str_len_max(char *s, size_t max)
{
    size_t len = 0;
    while(*s++ && len < max)
    {
        ++len;
    }
    return(len);
}

void
ez_str_copy(char *src, char *dest)
{
    while(*src)
    {
        *dest++ = *src++;
    }
}

void
ez_str_copy_max(char *src, char *dest, size_t max)
{
    size_t i = 0;
    while(*src && i < max)
    {
        *dest++ = *src++;
        ++i;
    }
}

int
ez_str_decimal(char *s)
{
    int value = 0;
    int sign = 1;

    if(*s == '-')
    {
        sign = -1;
        ++s;
    }
    else if(*s == '+')
    {
        sign = 1;
        ++s;
    }

    while(ez_char_is_digit(*s))
    {
        int curr = (*s++ - '0');
        value *= 10;
        value += curr;
    }

    return(value*sign);
}

HANDLE *_ez_stdout;
HANDLE *_ez_stdout;
HANDLE *_ez_stdout;

void
ez_out_print(char *s)
{
    if(!_ez_stdout)
    {
        _ez_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    WriteFile(_ez_stdout, s, (DWORD)ez_str_len(s), 0, 0);
}

void
ez_out_println(char *s)
{
    ez_out_print(s);
    ez_out_print("\n");
}


int
ez_file_exists(char *pathname)
{
    int exists = 0;
    HANDLE file_handle;

    file_handle = CreateFileA(
        pathname, FILE_GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        exists = 1;
        CloseHandle(file_handle);
    }

    return(exists);
}

size_t
ez_file_size(char *pathname)
{
    HANDLE file_handle = 0;
    size_t file_size = 0;

    LARGE_INTEGER li;

    file_handle = CreateFileA(
        pathname, FILE_GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file_handle != INVALID_HANDLE_VALUE)
    {
        file_size = 0;
    }
    else
    {
        if(!GetFileSizeEx(file_handle, &li))
        {
            file_size = 0;
        }
        else
        {
            file_size = (size_t)li.QuadPart;
        }
    }

    if(file_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file_handle);
    }

    return(file_size);
}

/* TODO: What if size == NULL? */
char *
ez_file_read_text(char *pathname, size_t *size)
{
    char *content = 0;
    char *content_ptr = 0;
    HANDLE file_handle;
    LARGE_INTEGER large_int;
    size_t bytes_to_read;
    uint32_t to_read, read;
    int error_occurred;

    *size = 0;

    file_handle = CreateFileA(
        pathname, FILE_GENERIC_READ, FILE_SHARE_READ,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(file_handle == INVALID_HANDLE_VALUE)
    {
        return(content);
    }

    if(GetFileSizeEx(file_handle, &large_int))
    {
        *size = large_int.QuadPart;

        content = (char *)ez_mem_alloc(*size);
        if(content)
        {
            bytes_to_read = *size;
            error_occurred = 0;
            content_ptr = content;
            do
            {
                if(bytes_to_read <= UINT32_MAX)
                {
                    to_read = (uint32_t)bytes_to_read;
                }
                else
                {
                    to_read = UINT32_MAX;
                }
                if(ReadFile(
                    file_handle, content_ptr,
                    to_read, (LPDWORD)&read, 0))
                {
                    bytes_to_read -= read;
                    content_ptr += read;
                }
                else
                {
                    error_occurred = 1;
                    ez_mem_free(content);
                    content = 0;
                    *size = 0;
                }
            } while((bytes_to_read > 0) && !(error_occurred));
        }

        CloseHandle(file_handle);
    }

    return(content);
}

void *
ez_file_read_bin(char *pathname, size_t *size)
{
    void *content = 0;
    content = (void *)ez_file_read_text(pathname, size);
    return(content);
}

void
ez_file_free(void *file_content)
{
    ez_mem_free(file_content);
}

int
ez_file_write(char *pathname, void *content, size_t size)
{
    int result = 0;
    HANDLE file_handle;
    uint32_t bytes_to_write;
    uint32_t bytes_written;
    int error_occurred;

    file_handle = CreateFileA(
        pathname, FILE_WRITE_DATA, FILE_SHARE_READ,
        0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(file_handle == INVALID_HANDLE_VALUE)
    {
        return(result);
    }

    error_occurred = 0;
    do
    {
        if(size >= UINT32_MAX)
        {
            bytes_to_write = UINT32_MAX;
        }
        else
        {
            bytes_to_write = (uint32_t)size;
        }
        if(WriteFile(
            file_handle, content,
            bytes_to_write, (LPDWORD)&bytes_written, 0))
        {
            size -= bytes_written;
        }
        else
        {
            error_occurred = 1;
        }
    } while((size > 0) && !(error_occurred));

    result = !error_occurred;

    CloseHandle(file_handle);

    return(result);
}

int
ez_file_append(char *pathname, void *content, size_t size)
{
    int result = 0;
    HANDLE file_handle;
    uint32_t bytes_to_write;
    uint32_t bytes_written;
    int error_occurred;
    uint8_t *content_ptr;

    if(ez_file_exists(pathname))
    {
        file_handle = CreateFileA(
            pathname, FILE_APPEND_DATA, FILE_SHARE_READ,
            0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if(file_handle == INVALID_HANDLE_VALUE)
        {
            return(result);
        }

        error_occurred = 0;
        content_ptr = (uint8_t *)content;
        do
        {
            if(size >= UINT32_MAX)
            {
                bytes_to_write = UINT32_MAX;
            }
            else
            {
                bytes_to_write = (uint32_t)size;
            }
            if(WriteFile(
                file_handle, content_ptr,
                bytes_to_write, (LPDWORD)&bytes_written, 0))
            {
                size -= bytes_written;
                content_ptr += bytes_written;
            }
            else
            {
                error_occurred = 1;
            }
        } while((size > 0) && !(error_occurred));

        result = !error_occurred;

        CloseHandle(file_handle);
    }
    else
    {
        result = ez_file_write(pathname, content, size);
    }

    return(result);
}

#else

#error "Unsupported platform"

#endif

#endif
