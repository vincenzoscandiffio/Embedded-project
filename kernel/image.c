/*
 * The read-only program image, kernel side: see kernel/image.h.
 */
#include <stdint.h>
#include "kernel/image.h"

#define IMG1_MAGIC          0x31474d49u   /* "IMG1" little-endian (6.1) */
#define IMG1_VERSION        1u
#define IMG1_HEADER_SIZE    20u
#define IMG1_ENTRY_SIZE     24u
#define IMG1_NAME_SIZE      16u
#define IMG1_MIN_FILE_SIZE  40u
#define IMG1_MAX_FILES      32u
#define IMG1_MAX_IMAGE_SIZE 0x00200000u

/* IMAGE_BASE is normally __fs_image_start, the linker symbol for FLASH
 * 0x00200000 (scripts/mps2_m3.ld). A test build can instead define
 * IMAGE_BASE as a host-side byte array on the compiler command line
 * (-DIMAGE_BASE=some_buffer) to exercise this file's logic against
 * synthetic images without a target or QEMU; production behavior is
 * unchanged either way. */
#ifndef IMAGE_BASE
extern const uint8_t __fs_image_start[];
#define IMAGE_BASE __fs_image_start
#endif

struct img1_header {
    uint32_t magic;
    uint32_t version;
    uint32_t image_size;
    uint32_t count;
    uint32_t directory_offset;
};
_Static_assert( sizeof( struct img1_header ) == IMG1_HEADER_SIZE,
                 "IMG1 header must be exactly 20 bytes" );

struct img1_dirent {
    char name[IMG1_NAME_SIZE];
    uint32_t offset;
    uint32_t size;
};
_Static_assert( sizeof( struct img1_dirent ) == IMG1_ENTRY_SIZE,
                 "IMG1 directory entry must be exactly 24 bytes" );

/* Is [p, p + len) fully inside [start, end)? Same overflow-safe shape as
 * kernel/syscall.c's in_range(): written so p + len can never wrap. */
static int in_range( uint32_t p, uint32_t len, uint32_t start, uint32_t end )
{
    return p >= start && p <= end && len <= end - p;
}

/* 1..15 characters from A-Z a-z 0-9 _ -, NUL-terminated within the
 * 16-byte field (6.1). Bytes after the first NUL are not checked: the
 * packer guarantees they are zero, and the kernel does not need to. */
static int name_ok( const char *name )
{
    unsigned i = 0;
    while ( i < IMG1_NAME_SIZE && name[i] != '\0' )
    {
        char c = name[i];
        int ok = ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' )
               || ( c >= '0' && c <= '9' ) || c == '_' || c == '-';
        if ( !ok )
            return 0;
        i++;
    }
    return i >= 1 && i < IMG1_NAME_SIZE;
}

/* Equal, comparing at most 16 bytes of each -- always safe since a
 * name produced by name_ok() has a NUL by position 15 at the latest. */
static int name_eq( const char *a, const char *b )
{
    for ( unsigned k = 0; k < IMG1_NAME_SIZE; k++ )
    {
        if ( a[k] != b[k] )
            return 0;
        if ( a[k] == '\0' )
            return 1;
    }
    return 1;
}

/* Parse and fully validate the image into `out` (holds at least
 * IMG1_MAX_FILES entries). Returns the entry count on success, -1 on
 * any violation of assignment.md 6.1. Everything is read straight out
 * of IMAGE_BASE; nothing here ever copies the image. */
static int parse( struct image_entry out[] )
{
    const struct img1_header *hdr = ( const struct img1_header * ) IMAGE_BASE;

    if ( hdr->magic != IMG1_MAGIC || hdr->version != IMG1_VERSION )
        return -1;
    if ( hdr->image_size < IMG1_HEADER_SIZE || hdr->image_size > IMG1_MAX_IMAGE_SIZE )
        return -1;
    if ( hdr->count < 1 || hdr->count > IMG1_MAX_FILES )
        return -1;
    if ( ( hdr->directory_offset & 3u ) != 0 || hdr->directory_offset < IMG1_HEADER_SIZE )
        return -1;

    uint32_t dir_bytes = hdr->count * IMG1_ENTRY_SIZE;  /* count <= 32: never overflows */
    if ( !in_range( hdr->directory_offset, dir_bytes, 0, hdr->image_size ) )
        return -1;
    /* directory_offset and dir_bytes are both multiples of 4, so their
     * sum already is too: no extra rounding needed here (6.1). */
    uint32_t contents_start = hdr->directory_offset + dir_bytes;

    const struct img1_dirent *dir = ( const struct img1_dirent * )
        ( IMAGE_BASE + hdr->directory_offset );

    for ( uint32_t i = 0; i < hdr->count; i++ )
    {
        if ( !name_ok( dir[i].name ) )
            return -1;
        if ( ( dir[i].offset & 3u ) != 0 || dir[i].offset < contents_start )
            return -1;
        if ( dir[i].size < IMG1_MIN_FILE_SIZE )
            return -1;
        if ( !in_range( dir[i].offset, dir[i].size, contents_start, hdr->image_size ) )
            return -1;

        out[i].name = dir[i].name;
        out[i].data = IMAGE_BASE + dir[i].offset;
        out[i].size = dir[i].size;
    }

    /* No duplicate names, no two files overlapping. O(n^2) is fine:
     * at most 32 entries, checked only when the image changes (never,
     * at runtime) or a lookup is requested. */
    for ( uint32_t i = 0; i < hdr->count; i++ )
    {
        for ( uint32_t j = i + 1; j < hdr->count; j++ )
        {
            if ( name_eq( out[i].name, out[j].name ) )
                return -1;

            uint32_t a = ( uint32_t ) ( out[i].data - IMAGE_BASE );
            uint32_t b = ( uint32_t ) ( out[j].data - IMAGE_BASE );
            if ( a < b + out[j].size && b < a + out[i].size )
                return -1;
        }
    }

    return ( int ) hdr->count;
}

static struct image_entry entries[IMG1_MAX_FILES];

static int refresh( void )
{
    return parse( entries );
}

int image_valid( void )
{
    return refresh() >= 0;
}

int image_count( void )
{
    return refresh();
}

const struct image_entry *image_entry_at( unsigned index )
{
    int count = refresh();
    if ( count < 0 || index >= ( unsigned ) count )
        return 0;
    return &entries[index];
}

const struct image_entry *image_find( const char *name )
{
    int count = refresh();
    if ( count < 0 )
        return 0;
    for ( int i = 0; i < count; i++ )
    {
        if ( name_eq( entries[i].name, name ) )
            return &entries[i];
    }
    return 0;
}
