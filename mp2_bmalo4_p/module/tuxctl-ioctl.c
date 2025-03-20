/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/* tells whether command has been acknowledged yet or not */
int ack;

/* LED condition status, formatted the same way as the MP2 documentation */
static unsigned long LED_buffer; 

/* Holds the button data as such 
	RIGHT | LEFT | DOWN | UP | C | B | A | START 
*/
static unsigned long button_buffer;

/* Set up a lock for the button*/
static spinlock_t sync_lock = SPIN_LOCK_UNLOCKED;

/* My helper functions*/
int tux_init(struct tty_struct* tty);
int tux_buttons(struct tty_struct* tty, unsigned long arg);
int tux_set_led(struct tty_struct* tty, unsigned long arg);

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned a, b, c;
	unsigned long flag;
	char RDLU; 
	char moves;
	char directions;
    a = packet[0]; 
    b = packet[1]; // packet byte b contains the 1XXXCBAS info so we load these values to their mappings respectively
    c = packet[2]; // packet byte c contains the 1XXXRDLU info so we load these values to their mappings respectively

	switch (a)
	{
	case MTCP_ACK:
		/* Once recive the ACK, set ack to 1. */
		ack = 1;
		break;
	case MTCP_BIOC_EVENT:
		/* critical section
		 * RIGHT | LEFT | DOWN | UP | C | B | A | START
		 */
		RDLU = (~c)&0x0F;
		moves = ((~b)&0x0F);
		/* This sets up the directions byte to be formated the way we want 
			we used to have */
		directions = (((RDLU&0x02)<<1)|((RDLU&0x04)>>1)| (RDLU&0x09))<<4;
		spin_lock_irqsave(&sync_lock,flag);
		button_buffer = directions|moves; //accessing shared memory so we have to lock so we can edit button_buffer un-interrupted
		spin_unlock_irqrestore(&sync_lock,flag);
		break;
	case MTCP_RESET:
		tux_init(tty);
		tux_set_led(tty, LED_buffer);
		break;
	default:
		return;
	}
	return;
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{
    switch (cmd) {
	case TUX_INIT:
		return tux_init(tty);
	case TUX_BUTTONS:
		return tux_buttons(tty,arg);
	case TUX_SET_LED:
		return tux_set_led(tty,arg);
	case TUX_LED_ACK:
		return 0;
	case TUX_LED_REQUEST:
		return 0;
	case TUX_READ_LED:
		return 0;
	default:
		return -EINVAL;
    }
}

/*
 * tux_init
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty-struct tty_struct*
 *   OUTPUTS: none
 *   RETURN VALUE:0 success and -EINVAL fail
 *   SIDE EFFECTS: set the initial value to all the variable of TUX
 */ 
int tux_init(struct tty_struct* tty){
	/* Initialize the LED and BUTTON condition to 0*/
	char coms[2] = {MTCP_BIOC_ON, MTCP_LED_USR};
	unsigned long flag;
	ack = 0; //set that we have not acknowledged this command
	LED_buffer = 0;

	/* critical section */
	spin_lock_irqsave(&sync_lock,flag);
	button_buffer = 0;
	spin_unlock_irqrestore(&sync_lock,flag);

	/* Go into user mode AND call BIOC_ON so we can control the LEDS and Enable interrupt generation */
	if(tuxctl_ldisc_put(tty, &coms[0],1) || tuxctl_ldisc_put(tty, &coms[1],1)){
	 	return -EINVAL;
	}

	return 0;
}

/*
 * tux_buttons
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty -struct tty_struct*
 * 			 arg - unsigned long, a pointer to a 32-bit integer, which represents the state of button
 *   OUTPUTS: none
 *   RETURN VALUE:0 success and -EINVAL fail
 *   SIDE EFFECTS: essentially copy the buttons state to userspace
 */ 
int tux_buttons(struct tty_struct* tty, unsigned long arg){
	int copied;
	unsigned long flag;

	/* ensure the argument is valid */
	if(&arg == NULL){
		return -EINVAL;
	}
	
	spin_lock_irqsave(&sync_lock,flag);
	copied = copy_to_user((uint32_t*)arg,(uint32_t*)(&button_buffer),sizeof(uint32_t)); //copying shared memory to user-space needs lock FS
	spin_unlock_irqrestore(&sync_lock,flag);

	return (copied) ? -EINVAL : 0; //fails if copy to user failed
}

const char segmentPatterns[16] = {
    0xE7, // 0
    0x06, // 1
    0xCB, // 2
    0x8F, // 3
    0x2E, // 4
    0xAD, // 5
    0xED, // 6
    0x86, // 7
    0xEF, // 8
    0xAE, // 9
    0xEE, // A
    0x8F, // B
    0xE0, // C Might look weird
    0xE1, // D also might be wierd
    0xE9, // E
    0xE8  // F
};

/*
 * tux_set_led
 *   DESCRIPTION: initialize the tux condition 
 *   INPUTS: tty -struct tty_struct*
 * 			 arg - unsigned long,  32-bit integer, which represents the state of LED
 *   OUTPUTS: none
 *   RETURN VALUE:0 success and -EINVAL fail
 *   SIDE EFFECTS: set the LEDs to specfied by arg
 */ 
int tux_set_led(struct tty_struct* tty, unsigned long arg){
	unsigned int Input_size;
	unsigned int ones, tens, hunds, thous, i;
	/* Mapping from 7-segment to bits
 	The 7-segment display is:
		  _A
		F| |B
		  -G
		E| |C
		  -D .dp

 	The map from bits to segments is:
 
 	__7___6___5___4____3___2___1___0__
 	| A | E | F | dp | G | C | B | D | 
 	+---+---+---+----+---+---+---+---+
 
 	Arguments: >= 1 bytes
		byte 0 - Bitmask of which LED's to set:

		__7___6___5___4____3______2______1______0___
 		| X | X | X | X | LED3 | LED2 | LED1 | LED0 | 
 		----+---+---+---+------+------+------+------+
 	byte 0 is MTCP_LED_SET, 1 is the above byte, 2+ are the segment bytes LED0 ->->-> LED3
	NOTE THAT THIS IS GRABBED STRAIT FROM THE MP2 DOCUMENTATION FOR FASTER REFERENCING */

	unsigned char LED_buff[6]; //6 is the most bytes the command could need
	/* If the ack is 0, return */
	if (!ack){
	 	return 0;
	}
	ack = 0;

	/* Set up the LED_buff*/
	LED_buff[0] = MTCP_LED_USR;
	tuxctl_ldisc_put(tty,LED_buff,1);
	LED_buff[0] = MTCP_LED_SET;
	LED_buff[1] = (arg >> 16) & 0x0F;

	Input_size = 2;
	/* go through one-hot encoding and find how many LED's we want to write to */
	for (i = 0; i < 4; i++){
		if (LED_buff[1]>>i & 0x01){
			Input_size++;
		}
	}
	/* 7-segment display bytes */
	/* the way I pass arg is with the low 16 bits is by splitting the 4 bits sets up like... 
		1. tens places mins 2. ones places mins 3. tens place secs 4. ones place secs */
	ones = (char)(arg & 0x0000000F); 
	tens = (char)((arg & 0x000000F0)>>4);
	hunds = (char)((arg & 0x00000F00)>>8);
	thous = (char)((arg & 0x0000F000)>>12);
	/* now to map these to 7-segment drawings and add decimal point (thats what the rightside logic is)*/
	LED_buff[2] = segmentPatterns[ones] | (char)((arg & 0x01000000)>>20);
	LED_buff[3] = segmentPatterns[tens] | (char)((arg & 0x02000000)>>21);
	LED_buff[4] = segmentPatterns[hunds] | (char)((arg & 0x04000000)>>22);
	LED_buff[5] = segmentPatterns[thous] | (char)((arg & 0x08000000)>>23);

	if(tuxctl_ldisc_put(tty,LED_buff,Input_size)){ 
		return -EINVAL;
	}
	LED_buffer = arg; //save the arg for reset 
	
	return 0;

}
