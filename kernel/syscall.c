/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * System calls, kernel side: find which service `svc #n` asked for,
 * run it, and hand the result back to the caller.
 *
 * Everything here runs inside the SVCall exception, so it is
 * privileged: it may touch the drivers and the hardware registers.
 *
 * One service does not return to its caller: SYS_START_TASKS (main()
 * starts the first task). It enters a task instead, so svc_dispatch()
 * returns the EXC_RETURN value that ends the exception (see
 * kernel/svc_entry.s).
 *
 * Since week 5, some services give the CPU away: yield, delay,
 * delay_until and exit. They do not switch here. They only change the
 * task's state and make PendSV pending (task_request_switch()). PendSV
 * has a lower priority than SVCall (0xE0 against 0x80), so it waits:
 * it runs right after this exception ends, and switches then. That is
 * why exit can now be an ordinary entry of the table: by the time the
 * task would resume after its `svc`, PendSV has already switched away,
 * and a FINISHED task is never chosen again.
 */

#include <stdint.h>
#include "kernel/syscall.h"
#include "lib/syscalls.h"
#include "drivers/uart.h"
#include "drivers/systick.h"
#include "kernel/task.h"
#include "kernel/trace.h"
#include "kernel/image.h"
#include "kernel/kprintf.h"

/* Every service has the same shape: up to three arguments (the
 * caller's r0, r1, r2) and one result (which becomes the caller's r0).
 * A service that needs fewer arguments simply ignores the others.
 * syscall_fn is the type "pointer to such a function". */
typedef uint32_t ( *syscall_fn )( uint32_t a0, uint32_t a1, uint32_t a2 );

/* Is [p, p + len) fully inside [start, end)?
 * Written as `len <= end - p` so that p + len can never wrap around. */
static int in_range( uint32_t p, uint32_t len, uint32_t start, uint32_t end )
{
    return p >= start && p <= end && len <= end - p;
}

/* SYS_WRITE: a0 = buf, a1 = len. Put as many bytes as fit into the
 * UART send buffer and return how many were taken.
 * It must never wait for room. We are inside the SVCall handler
 * (priority 0x80), and the UART send interrupt that empties the buffer
 * has a less urgent priority (0xC0): it cannot run until we return. If
 * we waited here for room, nothing would ever make room, and the
 * system would freeze. So we return the count, possibly 0, and the
 * caller tries again later. */
static uint32_t sys_write_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a2;                                  /* not used */
    const char *buf = ( const char * ) a0;      /* the number is really an address */
    unsigned len = a1;

    /* A task must not make the kernel read memory the task itself may
     * not read. Accept the buffer only inside its own partition or
     * inside FLASH (string constants). The kernel itself is trusted. */
    const struct tcb *t = tcb_current;
    if ( t != 0
         && !in_range( a0, len, t->part_start, t->part_end )
         && !in_range( a0, len, 0x00000000, 0x00400000 ) )
        return ( uint32_t ) -1;

    return uart_write( buf, len );              /* bytes accepted, 0..len */
}

/* SYS_READ: no arguments. Return one received byte, or -1 if none. */
static uint32_t sys_read_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    /* -1 becomes 0xFFFFFFFF here; the caller's `int` turns it back into -1. */
    return ( uint32_t ) uart_getc_polled();
}

/* SYS_UPTIME: no arguments. Return the milliseconds since start. */
static uint32_t sys_uptime_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    return systick_uptime_ms();
}

/* The scheduler services below only make sense for a task. Before the
 * first task starts (tcb_current == 0), they answer -1. */

/* SYS_YIELD: no arguments. The calling task gives up the rest of its
 * time slice: it stays READY, and the next READY task runs. If no other
 * task is READY, the scheduler picks the caller again. */
static uint32_t sys_yield_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    if ( tcb_current == 0 )
        return ( uint32_t ) -1;
    task_request_switch();                      /* PendSV runs after this exception */
    return 0;
}

/* SYS_DELAY: a0 = ticks. Block the caller for that many ticks, counted
 * from now. A delay of 0 is just a yield. */
static uint32_t sys_delay_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a1;                                  /* not used */
    (void) a2;
    if ( tcb_current == 0 )
        return ( uint32_t ) -1;
    if ( a0 == 0 )
        task_request_switch();                  /* delay(0) = yield */
    else
        task_block_until( tick_count + a0 );    /* READY again at tick now + a0 */
    return 0;
}

/* SYS_DELAY_UNTIL: a0 = last_wake (an address in the task's
 * partition), a1 = period. Block the caller until tick *last_wake +
 * period, and store that tick in *last_wake for the next call. Unlike
 * SYS_DELAY, the time the task spent working does not push the next
 * wake-up later: the task wakes at last_wake + period, + 2 * period,
 * ... exactly, which is what a periodic task needs. */
static uint32_t sys_delay_until_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a2;                                  /* not used */
    const struct tcb *t = tcb_current;
    if ( t == 0 )
        return ( uint32_t ) -1;

    /* The kernel is going to read and WRITE *last_wake. As for
     * SYS_WRITE, accept the address only inside the caller's own
     * partition, or a task could make the kernel write anywhere. It
     * must also be a multiple of 4, as every uint32_t is. */
    if ( !in_range( a0, sizeof( uint32_t ), t->part_start, t->part_end )
         || ( a0 & 3 ) != 0 )
        return ( uint32_t ) -1;

    uint32_t *last_wake = ( uint32_t * ) a0;    /* the number is really an address */
    uint32_t wake = *last_wake + a1;            /* the next wake-up tick */
    *last_wake = wake;                          /* remembered for the next period */

    /* Is wake still in the future? Same wrap-safe test as in
     * task_increment_tick(). If the task is late (wake already passed),
     * it does not block at all: it goes on at once. */
    if ( ( int32_t ) ( wake - tick_count ) > 0 )
        task_block_until( wake );
    return 0;
}

/* SYS_EXIT: no arguments. The calling task is finished: it is marked
 * FINISHED and never chosen again. The result is never seen. */
static uint32_t sys_exit_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    if ( tcb_current == 0 )
        return ( uint32_t ) -1;
    task_finish_current();                      /* PendSV switches away after this exception */
    return 0;
}

/* SYS_TICK_COUNT: no arguments. Return the ticks since the scheduler
 * started (one tick = 10 ms). */
static uint32_t sys_tick_count_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    return tick_count;
}

/* SYS_TRACE_DUMP: no arguments. Print the scheduler trace and its
 * Gantt chart on the UART (kernel/trace.c). */
static uint32_t sys_trace_dump_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;                                  /* not used */
    (void) a1;
    (void) a2;
    trace_print();
    return 0;
}

/* ------------------------------------------------------------------ */
/* SYS_IMAGE_COUNT (11): number of directory entries, or -1 (R5, 6.5) */
/* ------------------------------------------------------------------ */
static uint32_t sys_image_count_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a0;
    (void) a1;
    (void) a2;
    return ( uint32_t ) image_count();          /* 1..32 or -1 */
}

/* ------------------------------------------------------------------ */
/* SYS_IMAGE_NAME (12): copy one entry's name into the caller's        */
/* buffer.  a0 = index, a1 = name destination, a2 = capacity (6.5).   */
/* ------------------------------------------------------------------ */
static uint32_t sys_image_name_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    const struct tcb *t = tcb_current;
    if ( t == 0 )
        return ( uint32_t ) -1;

    unsigned capacity = a2;
    /* capacity must be 1..16 (6.5). */
    if ( capacity < 1 || capacity > 16 )
        return ( uint32_t ) -1;

    /* The destination must lie entirely inside the caller's partition.
     * FLASH is NOT a valid destination (6.5). */
    if ( !in_range( a1, capacity, t->part_start, t->part_end ) )
        return ( uint32_t ) -1;

    /* Get the entry from the validated image. */
    const struct image_entry *e = image_entry_at( a0 );
    if ( e == 0 )
        return ( uint32_t ) -1;

    /* Copy as much of the name as fits, always NUL-terminated. */
    char *dst = ( char * ) ( uintptr_t ) a1;
    const char *src = e->name;
    unsigned i = 0;
    while ( i + 1 < capacity && src[i] != '\0' )
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    /* If the full name (with NUL) did not fit, return -1 (6.5). */
    unsigned name_len = 0;
    while ( src[name_len] != '\0' )
        name_len++;
    if ( name_len + 1 > capacity )
        return ( uint32_t ) -1;

    return 0;
}

/* ------------------------------------------------------------------ */
/* SYS_PROGRAM_RUN (13): load and start a named program (R5, R6, 6.5) */
/* a0 = name pointer.  Returns task id, or -1..-4 on error.           */
/* ------------------------------------------------------------------ */

/* Slot geometry, mirroring kernel/task.c. */
#define SLOT_SIZE   4096u
#define SLOT_CODE   3072u   /* max code + data + BSS */
#define SLOT_STACK  1024u   /* stack at the top of the slot */

extern uint32_t __program_slots_start[];

/* Read-only ELF structures needed by the big-version kernel loader. */

/* ELF32 types. */
#define EI_NIDENT       16
#define ET_EXEC         2
#define EM_ARM          40
#define PT_LOAD         1
#define SHT_NOBITS      8
#define SHT_REL         9
#define SHT_RELA        4
#define SHF_ALLOC       0x2
#define R_ARM_ABS32     2
#define R_ARM_THM_CALL  10
#define R_ARM_THM_JUMP24 30
#define ELF_MAGIC       0x464c457fu  /* "\x7fELF" little-endian */

struct elf32_ehdr {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};
_Static_assert( sizeof( struct elf32_ehdr ) == 52, "ELF header must be 52 bytes" );

struct elf32_phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};
_Static_assert( sizeof( struct elf32_phdr ) == 32, "program header must be 32 bytes" );

struct elf32_shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};
_Static_assert( sizeof( struct elf32_shdr ) == 40, "section header must be 40 bytes" );

struct elf32_sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
};
_Static_assert( sizeof( struct elf32_sym ) == 16, "symbol must be 16 bytes" );

struct elf32_rel {
    uint32_t r_offset;
    uint32_t r_info;
};
_Static_assert( sizeof( struct elf32_rel ) == 8, "REL entry must be 8 bytes" );

#define ELF32_R_SYM(i)   ((i) >> 8)
#define ELF32_R_TYPE(i)  ((i) & 0xffu)

/* Is [off, off + len) fully inside [0, limit)?  Overflow-safe. */
static int fits( uint32_t off, uint32_t len, uint32_t limit )
{
    return off <= limit && len <= limit - off;
}

/* Load a program from a validated image entry into a given slot.
 * Returns a task id >= 0 on success, or -4 on any file / format error.
 * NOTE (R9): All checks are performed in Phase 1 BEFORE modifying any byte
 * of the slot. Only once everything is validated do we copy and relocate. */
static int load_elf( const struct image_entry *e, unsigned slot )
{
    uint32_t base = ( uint32_t ) ( ( uintptr_t ) __program_slots_start + slot * SLOT_SIZE );
    uint8_t *ram = ( uint8_t * ) ( uintptr_t ) base;
    const uint8_t *file = e->data;
    uint32_t file_size = e->size;

    /* ================================================================ */
    /* PHASE 1: Complete Validation (no memory writes to the slot)      */
    /* ================================================================ */

    /* 1. ELF Header validation */
    if ( file_size < 52 )
        return -4;
    const struct elf32_ehdr *eh = ( const struct elf32_ehdr * ) file;

    if ( eh->e_ident[0] != 0x7f || eh->e_ident[1] != 'E'
      || eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F' )
        return -4;
    if ( eh->e_ident[4] != 1 )  return -4;  /* 32-bit (ELFCLASS32) */
    if ( eh->e_ident[5] != 1 )  return -4;  /* little-endian (ELFDATA2LSB) */
    if ( eh->e_ident[6] != 1 )  return -4;  /* EV_CURRENT */
    if ( eh->e_type != ET_EXEC )   return -4;
    if ( eh->e_machine != EM_ARM ) return -4;
    if ( eh->e_version != 1 )      return -4;
    if ( eh->e_ehsize != 52 )      return -4;
    if ( eh->e_phentsize != 32 )   return -4;

    /* Program header table must be inside the file and after ELF header. */
    if ( eh->e_phnum == 0 || eh->e_phoff < 52 )
        return -4;
    if ( !fits( eh->e_phoff, ( uint32_t ) eh->e_phnum * 32, file_size ) )
        return -4;

    /* Section header table (if present) must be inside the file and after ELF header. */
    uint16_t shnum = eh->e_shnum;
    if ( shnum > 0 )
    {
        if ( eh->e_shentsize != 40 || eh->e_shoff < 52 )
            return -4;
        if ( !fits( eh->e_shoff, ( uint32_t ) shnum * 40, file_size ) )
            return -4;
    }

    /* 2. Program headers (segments) validation */
    const struct elf32_phdr *ph =
        ( const struct elf32_phdr * ) ( file + eh->e_phoff );

    int has_load = 0;
    uint32_t min_vaddr = 0xFFFFFFFFu;
    uint32_t load_end_file = 0;   /* end of file-backed content in virtual memory */
    uint32_t load_end_mem  = 0;   /* end of memory (includes BSS) */

    for ( uint16_t i = 0; i < eh->e_phnum; i++ )
    {
        if ( ph[i].p_type != PT_LOAD )
            continue;
        has_load = 1;

        /* File content must fit in the ELF file. */
        if ( !fits( ph[i].p_offset, ph[i].p_filesz, file_size ) )
            return -4;
        /* memsz must be at least filesz (the remainder is BSS). */
        if ( ph[i].p_memsz < ph[i].p_filesz )
            return -4;
        /* Virtual memory range must fit in the lower 3072 bytes (6.4). */
        if ( !fits( ph[i].p_vaddr, ph[i].p_memsz, SLOT_CODE ) )
            return -4;

        if ( ph[i].p_vaddr < min_vaddr )
            min_vaddr = ph[i].p_vaddr;

        uint32_t seg_file_end = ph[i].p_vaddr + ph[i].p_filesz;
        uint32_t seg_mem_end  = ph[i].p_vaddr + ph[i].p_memsz;
        if ( seg_file_end > load_end_file )
            load_end_file = seg_file_end;
        if ( seg_mem_end > load_end_mem )
            load_end_mem = seg_mem_end;
    }

    if ( !has_load || min_vaddr != 0 )
        return -4;
    if ( load_end_mem > SLOT_CODE )
        return -4;

    /* Check that PT_LOAD segments do not overlap in memory. */
    for ( uint16_t i = 0; i < eh->e_phnum; i++ )
    {
        if ( ph[i].p_type != PT_LOAD )
            continue;
        uint32_t start_i = ph[i].p_vaddr;
        uint32_t end_i   = ph[i].p_vaddr + ph[i].p_memsz;
        for ( uint16_t j = i + 1; j < eh->e_phnum; j++ )
        {
            if ( ph[j].p_type != PT_LOAD )
                continue;
            uint32_t start_j = ph[j].p_vaddr;
            uint32_t end_j   = ph[j].p_vaddr + ph[j].p_memsz;
            if ( start_i < end_j && start_j < end_i )
                return -4;
        }
    }

    /* 3. Section headers and relocations validation */
    const struct elf32_shdr *sh = 0;
    if ( shnum > 0 )
        sh = ( const struct elf32_shdr * ) ( file + eh->e_shoff );

    for ( uint16_t i = 0; i < shnum; i++ )
    {
        /* Each section's data must fit in the file. */
        if ( sh[i].sh_type != SHT_NOBITS )
        {
            if ( !fits( sh[i].sh_offset, sh[i].sh_size, file_size ) )
                return -4;
        }

        if ( sh[i].sh_type == SHT_RELA )
            return -4;  /* RELA not accepted (6.3) */

        if ( sh[i].sh_type != SHT_REL )
            continue;

        if ( sh[i].sh_entsize != 8 || ( sh[i].sh_size % 8 ) != 0 )
            return -4;

        uint32_t target_idx = sh[i].sh_info;
        if ( target_idx >= shnum )
            return -4;
        const struct elf32_shdr *target_sh = &sh[target_idx];

        /* Relocations for non-allocated sections (e.g. debug) are ignored. */
        if ( ( target_sh->sh_flags & SHF_ALLOC ) == 0 )
            continue;

        /* Allocated section relocations: symbol table check. */
        uint32_t symtab_idx = sh[i].sh_link;
        if ( symtab_idx >= shnum )
            return -4;
        const struct elf32_shdr *sym_sh = &sh[symtab_idx];
        if ( sym_sh->sh_entsize != 16 || ( sym_sh->sh_size % 16 ) != 0 )
            return -4;
        if ( !fits( sym_sh->sh_offset, sym_sh->sh_size, file_size ) )
            return -4;

        const struct elf32_sym *syms =
            ( const struct elf32_sym * ) ( file + sym_sh->sh_offset );
        uint32_t sym_count = sym_sh->sh_size / 16;

        uint32_t rel_count = sh[i].sh_size / 8;
        const struct elf32_rel *rels =
            ( const struct elf32_rel * ) ( file + sh[i].sh_offset );

        for ( uint32_t r = 0; r < rel_count; r++ )
        {
            uint32_t r_type = ELF32_R_TYPE( rels[r].r_info );
            uint32_t r_sym  = ELF32_R_SYM( rels[r].r_info );

            if ( r_sym >= sym_count )
                return -4;
            /* Defined symbols only (SHN_UNDEF = 0 refused). */
            if ( syms[r_sym].st_shndx == 0 )
                return -4;

            uint32_t site = rels[r].r_offset;

            if ( r_type == R_ARM_ABS32 )
            {
                /* Must be word-aligned and inside loaded memory. */
                if ( ( site & 3 ) != 0 || !fits( site, 4, load_end_mem ) )
                    return -4;

                /* Must lie inside a file-backed PT_LOAD segment (not in BSS). */
                int in_file = 0;
                uint32_t file_off = 0;
                for ( uint16_t p = 0; p < eh->e_phnum; p++ )
                {
                    if ( ph[p].p_type != PT_LOAD )
                        continue;
                    if ( site >= ph[p].p_vaddr && site + 4 <= ph[p].p_vaddr + ph[p].p_filesz )
                    {
                        in_file = 1;
                        file_off = ph[p].p_offset + ( site - ph[p].p_vaddr );
                        break;
                    }
                }
                if ( !in_file )
                    return -4;

                /* Check original word + base does not overflow 32 bits. */
                const uint8_t *w = file + file_off;
                uint32_t orig = ( uint32_t ) w[0]
                              | ( ( uint32_t ) w[1] << 8 )
                              | ( ( uint32_t ) w[2] << 16 )
                              | ( ( uint32_t ) w[3] << 24 );
                if ( orig + base < orig )
                    return -4;
            }
            else if ( r_type == R_ARM_THM_CALL || r_type == R_ARM_THM_JUMP24 )
            {
                /* Must be halfword-aligned and inside file-backed segment. */
                if ( ( site & 1 ) != 0 || !fits( site, 4, load_end_mem ) )
                    return -4;

                int in_file = 0;
                for ( uint16_t p = 0; p < eh->e_phnum; p++ )
                {
                    if ( ph[p].p_type != PT_LOAD )
                        continue;
                    if ( site >= ph[p].p_vaddr && site + 4 <= ph[p].p_vaddr + ph[p].p_filesz )
                    {
                        in_file = 1;
                        break;
                    }
                }
                if ( !in_file )
                    return -4;
            }
            else
            {
                kprintf( "loader: unsupported relocation type %u\n", r_type );
                return -4;
            }
        }
    }

    /* 4. Entry point validation */
    uint32_t entry_raw = eh->e_entry;
    if ( ( entry_raw & 1 ) == 0 )
        return -4;  /* must be Thumb */
    if ( !fits( entry_raw & ~1u, 2, load_end_file ) )
        return -4;  /* must be inside code/file-backed content */

    /* ================================================================ */
    /* PHASE 2: Copying and Relocation (all validations passed)         */
    /* ================================================================ */

    /* Zero the program area (code, data, and BSS) so BSS and gaps are 0. */
    for ( uint32_t i = 0; i < SLOT_CODE; i++ )
        ram[i] = 0;

    /* Copy PT_LOAD segment file contents. */
    for ( uint16_t i = 0; i < eh->e_phnum; i++ )
    {
        if ( ph[i].p_type != PT_LOAD )
            continue;
        const uint8_t *src = file + ph[i].p_offset;
        uint8_t *dst = ram + ph[i].p_vaddr;
        for ( uint32_t k = 0; k < ph[i].p_filesz; k++ )
            dst[k] = src[k];
    }

    /* Apply R_ARM_ABS32 relocations in the slot. */
    if ( shnum > 0 )
    {
        for ( uint16_t i = 0; i < shnum; i++ )
        {
            if ( sh[i].sh_type != SHT_REL )
                continue;
            uint32_t target_idx = sh[i].sh_info;
            if ( ( sh[target_idx].sh_flags & SHF_ALLOC ) == 0 )
                continue;

            uint32_t rel_count = sh[i].sh_size / 8;
            const struct elf32_rel *rels =
                ( const struct elf32_rel * ) ( file + sh[i].sh_offset );

            for ( uint32_t r = 0; r < rel_count; r++ )
            {
                uint32_t r_type = ELF32_R_TYPE( rels[r].r_info );
                if ( r_type == R_ARM_ABS32 )
                {
                    uint32_t site = rels[r].r_offset;
                    uint32_t *word = ( uint32_t * ) ( uintptr_t ) ( ram + site );
                    *word += base;
                }
            }
        }
    }

    /* Entry point and stack setup. */
    void ( *entry_fn )( void ) = ( void ( * )( void ) ) ( uintptr_t ) ( base + entry_raw );
    uint32_t *stack_top = ( uint32_t * ) ( uintptr_t ) ( base + SLOT_SIZE );

    return task_program_create( slot, e->name, entry_fn, stack_top, 1 );
}

static uint32_t sys_program_run_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a1;
    (void) a2;
    const struct tcb *t = tcb_current;
    if ( t == 0 )
        return ( uint32_t ) -1;

    /* Validate the name pointer: the string must be non-empty, NUL-
     * terminated within 16 bytes, and every byte in the caller's
     * partition or in FLASH 0x00000000..0x003fffff (6.5). */
    const char *name = ( const char * ) ( uintptr_t ) a0;
    unsigned len = 0;
    int found_nul = 0;
    for ( unsigned k = 0; k < 16; k++ )
    {
        uint32_t addr = a0 + k;
        if ( !in_range( addr, 1, t->part_start, t->part_end )
          && !in_range( addr, 1, 0x00000000, 0x00400000 ) )
            return ( uint32_t ) -1;
        if ( name[k] == '\0' )
        {
            found_nul = 1;
            break;
        }
        len++;
    }
    if ( !found_nul || len == 0 )
        return ( uint32_t ) -1;

    /* Look up the program in the image. */
    if ( !image_valid() )
        return ( uint32_t ) -1;
    const struct image_entry *e = image_find( name );
    if ( e == 0 )
        return ( uint32_t ) -2;

    /* Find the first free slot. */
    int slot = -1;
    for ( unsigned s = 0; s < PROGRAM_SLOT_COUNT; s++ )
    {
        if ( task_program_slot_free( s ) )
        {
            slot = ( int ) s;
            break;
        }
    }
    if ( slot < 0 )
        return ( uint32_t ) -3;

    /* Load the program (ELF, big version). */
    int result = load_elf( e, ( unsigned ) slot );
    return ( uint32_t ) result;
}

/* Wait bookkeeping is supplied even though the untouched base has no loader.
 * Student loader code supplies SYS_PROGRAM_RUN and creates program tasks. */
static uint32_t sys_program_wait_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a1;
    (void) a2;
    uint32_t *frame;
    __asm volatile ( "mrs %0, psp" : "=r" ( frame ) );
    return (uint32_t) task_wait_program( a0, &frame[0] );
}

/* The dispatch table: table[n] is the function for service n.
 * The [SYS_WRITE] = ... form puts each entry at its number, whatever
 * the order of the lines. Entry 0 is left empty (0): it is not a
 * service. Entry 5 (SYS_START_TASKS) is empty too: svc_dispatch
 * handles it itself, because it does not return to its caller. */
static const syscall_fn table[] = {
    [SYS_WRITE]       = sys_write_impl,
    [SYS_READ]        = sys_read_impl,
    [SYS_UPTIME]      = sys_uptime_impl,
    [SYS_EXIT]        = sys_exit_impl,
    [SYS_YIELD]       = sys_yield_impl,
    [SYS_DELAY]       = sys_delay_impl,
    [SYS_DELAY_UNTIL] = sys_delay_until_impl,
    [SYS_TRACE_DUMP]  = sys_trace_dump_impl,
    [SYS_TICK_COUNT]   = sys_tick_count_impl,
    [SYS_IMAGE_COUNT]  = sys_image_count_impl,
    [SYS_IMAGE_NAME]   = sys_image_name_impl,
    [SYS_PROGRAM_RUN]  = sys_program_run_impl,
    [SYS_PROGRAM_WAIT] = sys_program_wait_impl,
};

/* How many entries the table has (numbers 0..14). */
#define TABLE_SIZE ( sizeof( table ) / sizeof( table[0] ) )

/* EXC_RETURN bit 2: 1 if the caller was on the process stack (a task),
 * 0 if it was on the main stack (main(), the kernel). */
#define EXC_RETURN_PSP_BIT 0x4u

/* frame points at the 8 registers the hardware pushed when `svc` ran:
 *     frame[0] = r0     frame[4] = r12
 *     frame[1] = r1     frame[5] = lr
 *     frame[2] = r2     frame[6] = pc   (the instruction after `svc`)
 *     frame[3] = r3     frame[7] = xPSR
 * exc_return is the EXC_RETURN value of this exception. The function
 * returns the EXC_RETURN value SVC_Handler must use to end it. */
uint32_t svc_dispatch( uint32_t *frame, uint32_t exc_return )
{
    /* Which service? The number is not in a register: it is written
     * inside the `svc #n` instruction itself. That instruction is 2
     * bytes long and sits just before the saved pc (frame[6]), and its
     * first byte in memory holds n. So we look at the byte 2 bytes
     * before the saved pc:
     *     pc - 2:  n       (low byte of `svc #n`)
     *     pc - 1:  0xDF    (high byte: "this is an svc")
     *     pc:      the next instruction  */
    uint8_t n = ( ( uint8_t * ) frame[6] )[-2];

    /* Did a task ask (process stack), or main() (main stack)? */
    int from_task = ( exc_return & EXC_RETURN_PSP_BIT ) != 0;

    /* SYS_START_TASKS: kernel only. A task (on the process stack) that
     * tries it falls through to the table below and gets -1. */
    if ( n == SYS_START_TASKS && !from_task && tcb_current == 0 )
    {
        return task_start_first();      /* enter the first task */
    }

    /* An unknown number, or 0, or an empty table entry: answer -1. */
    if ( n >= TABLE_SIZE || table[n] == 0 )
    {
        frame[0] = ( uint32_t ) -1;
        return exc_return;              /* back to the caller */
    }

    /* Run the service with the caller's r0, r1, r2 as arguments, and
     * store its result in the saved r0. When the exception ends, the
     * hardware pops the frame back into the registers, and the caller
     * finds the result in r0. */
    frame[0] = table[n]( frame[0], frame[1], frame[2] );
    return exc_return;                  /* back to the caller, as it came */
}
