/*
 * The read-only program image, kernel side (assignment.md R3, 6.1): find
 * it at FLASH, check every IMG1 rule, and look programs up by name.
 * Nothing here ever copies the image -- every check and lookup reads
 * FLASH directly, through __fs_image_start (scripts/mps2_m3.ld).
 *
 * This module only validates the IMG1 container (header + directory). A
 * valid image can still contain a bad program file inside it -- checking
 * an individual program's own contents is the loader's job (R6), not
 * this one's.
 */
#ifndef IMAGE_H
#define IMAGE_H

/* One directory entry of a validated image. name is NUL-terminated,
 * 1-15 characters; [data, data + size) is the program file's exact
 * range in FLASH. */
struct image_entry {
    const char *name;
    const unsigned char *data;
    unsigned size;
};

/* Is the image at FLASH valid? Checks magic, version, every size/count/
 * offset range, every directory entry's name/offset/size, that no two
 * entries share a name, and that no two files overlap (6.1). */
int image_valid( void );

/* Number of directory entries (1..32), or -1 if the image is invalid.
 * Matches SYS_IMAGE_COUNT's contract exactly (6.5). */
int image_count( void );

/* Entry at directory position `index`, or 0 if the image is invalid or
 * index is out of range. */
const struct image_entry *image_entry_at( unsigned index );

/* Entry named `name`, or 0 if not found or the image is invalid. Reads
 * at most 16 bytes of `name` regardless of where its own NUL is, since
 * no valid program name is longer than 15 characters anyway. */
const struct image_entry *image_find( const char *name );

#endif /* IMAGE_H */
