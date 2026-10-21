/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Q6: a header of the program the Makefile builds.
 */
#ifndef BOARD_H
#define BOARD_H

/* The LED register of the board: a store to it lights the LEDs. */
#define LED_REG ( *( volatile unsigned * ) 0x40028000UL )

/* The number of LEDs. */
#define LED_COUNT 8u

#endif /* BOARD_H */
