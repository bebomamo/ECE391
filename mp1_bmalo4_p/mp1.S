
# mp1.S - missile-command version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct missile, 
    # struct missile is defined in rtc.h and mp1.h

	NEXT 	  = 0
	X	  = 4
	Y	  = 8
	VX	  = 12
	VY	  = 16
	DEST_X	  = 20
	DEST_Y	  = 24
	EXPLODED  = 28
	C	  = 32

    # Character to draw for an explosion - '@'

	EXPLOSION = 64


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:	.long 0x0	# Head of list pointer
base_alive:		.long 0x0	# Array of 3 bytes, plus a padding byte
mp1_score:		.long 0x0	# Players current score


# Data private to this  file

base_pic:     	    .string "/^^^\\"	# Picture of a live base
dead_base_pic: 	    .string "xxxxx"	# Picture of a dead base
crosshairs_x:		.long 0x0	# X-position of the crosshairs
crosshairs_y:		.long 0x0	# Y-position of the crosshairs

.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#	(1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#	(2) Redraw the bases - they may have been overwritten with missiles
#	(3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# walk down linked list and check for exploding, otherwise update missile position
		call update_missiles
		cmpl $-1, %eax
		je notify
		notified:
	# redraw cities in case they were destroyed, centered at row 24: cols 20, 40, and 60, overwrite over missiles
		movl base_alive, %ebx
		movl $0x00010000, %esi # value is to check for living base
		testl %ebx, %esi
		jnz right_alive # else right is dead
		jmp right_dead
		check_mid:
		movl $0x00000100, %esi
		testl %ebx, %esi
		jnz mid_alive
		jmp mid_dead
		check_left:
		movl $0x00000001, %esi
		testl %ebx, %esi
		jnz left_alive
		jmp left_dead

		right_alive:  # 3840 is row 24  "/^^^\"  "xxxxx"
		movl $116, %eax
		addl $3840, %eax
		movl $47, %ecx
		call mp1_poke
		addl $2, %eax
		movl $94, %ecx
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		movl $92, %ecx
		call mp1_poke
		jmp check_mid

		right_dead:
		movl $116, %eax
		addl $3840, %eax
		movl $120, %ecx
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		jmp check_mid

		mid_alive:
		movl $76, %eax
		addl $3840, %eax
		movl $47, %ecx
		call mp1_poke
		addl $2, %eax
		movl $94, %ecx
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		movl $92, %ecx
		call mp1_poke
		jmp check_left

		mid_dead:
		movl $76, %eax
		addl $3840, %eax
		movl $120, %ecx
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		jmp check_left

		left_alive:
		movl $36, %eax
		addl $3840, %eax
		movl $47, %ecx
		call mp1_poke
		addl $2, %eax
		movl $94, %ecx
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		call mp1_poke
		addl $2, %eax
		movl $92, %ecx
		call mp1_poke
		jmp cities_drawn

		left_dead:
		movl $36, %eax
		addl $3840, %eax
		movl $120, %ecx
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke
		addl $2, %eax 
		call mp1_poke

		cities_drawn:
	# redraw crosshair last to ensure the crosshair is always visible over everything else
		movl crosshairs_x, %eax
		movl crosshairs_y, %esi
		imul $160, %esi
		imul $2, %eax
		addl %esi, %eax
		movl $43, %ecx
		call mp1_poke

	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

		notify:
		call mp1_notify_user
		jmp notified
# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:
	# DONT CALLEE SAVE AND TEARDOWN HERE FOR SOME REASON
	# subroutine stack begins with 8(%esp) -> arg and 12(%esp) -> cmd(command number)
	movl 8(%esp), %ecx
	# assure cmd is <= 4 
	cmpl $4, %ecx
	jg invalid_cmd
	# assure cmd >=0
	cmpl $0, %ecx
	jl invalid_cmd
	# now we know that cmd is valid and can use the jump table (JUMP TABLE CALL)
	jmp *jump_table(,%ecx, 4)
	function0:
		jmp mp1_ioctl_startgame
	function1:
		jmp mp1_ioctl_addmissile
	function2:
		jmp mp1_ioctl_movexhairs
	function3:
		jmp mp1_ioctl_getstatus
	function4:
		jmp mp1_ioctl_endgame
	jump_table:
	.long function0, function1, function2, function3, function4

	invalid_cmd:
		andl $0, %eax
		addl $-1, %eax
		leave
		ret


# ----------------- Functions private to this file -------------------

# walks through the missiles and updates them, returns -1 if status changed(need to call mp1_user_notify), else returns 0
update_missiles:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# walk through linked list
		andl $0, %edi
		movl mp1_missile_list, %ebx
		cmpl $0, %ebx
		je zero_missiles
		next_missile:
	# 1st if missile is offscreen, missile should be erased from screen and removed and freed from the linked list
		movl $0xFFFF0000, %esi
		andl X(%ebx), %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		movl $0xFFFF0000, %ecx
		andl Y(%ebx), %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		shr %ecx
		cmpl $79, %esi
		jg offscreen
		cmpl $0, %esi
		jl offscreen
		cmpl $24, %ecx
		jg offscreen
		cmpl $0, %ecx
		jl offscreen
	# 2nd if missile reached destination or is exploding, call missile_explode with the missiles pointer as argument, and decrement exploded field for this missile
	# AND if exploded reaches zero, remove the missile-- otherwise draw the missile as the EXPLOSION character and hold its position
		cmpl $0, EXPLODED(%ebx)
		jne boom
		movl DEST_X(%ebx), %edx
		cmpl %esi, %edx # remember %esi is X text mode location
		jne no_boom
		movl DEST_Y(%ebx), %edx
		cmpl %ecx, %edx # remember %ecx is Y text mode location
		jne no_boom
		jmp boom
	# note: if the call to missile_explode returned that there was a change, call mp1_notify_user before tasklet returns
		no_boom:
	# 3rd if missile is non-exploding, in-frame, and moving towards destination, check if it moved text mode locations and redraw it
		# movl $0x0000FFFF, %esi
		# andl X(%ebx), %esi
		# addl VX(%ebx), %esi
		# cmpl $65536, %esi
		# jge erase_missile
		# movl $0x0000FFFF, %esi  /* turns out this section is useless, you have to erase missile and redraw everytime unconditionally */
		# andl Y(%ebx), %esi
		# addl VY(%ebx), %esi
		# cmpl $65536, %esi
		# jge erase_missile
		jmp erase_missile
	# NOTE THAT IN ALL CASES, REDRAW THE MISSILE SINCE IT COULD"VE BEEN OVERWRITTEN BY THE CROSSHAIR OR ANOTHER MISSILE
	# Also the reason I can do this is because 65535 is all 1s for 16 bits so the carry will workout to add to my row/col in text mode
		redraw_missile:
		movl VX(%ebx), %edx
		addl %edx, X(%ebx)
		movl VY(%ebx), %edx
		addl %edx, Y(%ebx)
		# REDRAW THE MISSILE BASED ON ITS C FIELD
		pushl %ebx
		call draw_missile
		addl $4, %esp
		# check for next missile
		cmpl $0, NEXT(%ebx)
		je zero_missiles
		movl NEXT(%ebx), %ebx
		jmp next_missile

		boom:
		pushl %ebx
		call missile_explode
		addl $4, %esp
		cmpl $0, %eax # did the explosion cause another?
		je no_chain_reaction
		movl $-1, %edi # flag to return -1 to notify user of a change
		no_chain_reaction:
		subl $1, EXPLODED(%ebx)
		jz explosion_finished
		# DRAW EXPLOSION CHARACTER AT THE MISSILE LOCATION
		movl $64, %edx
		movl %edx, C(%ebx) # '@' redundancy but I don't care
		pushl %ebx
		call draw_missile
		addl $4, %esp
		# check for next missile
		cmpl $0, NEXT(%ebx)
		je zero_missiles
		movl NEXT(%ebx), %ebx
		jmp next_missile

		explosion_finished:
		# label exists for clarity but does the same as offscreen
		jmp offscreen

		erase_missile:
		# ERASE THE MISSILES PREVIOUS LOCATION
		pushl C(%ebx)
		movl $32, C(%ebx) # ' '
		pushl %ebx
		call draw_missile
		addl $4, %esp
		popl C(%ebx)
		# now go back and draw the missile in the new spot
		jmp redraw_missile 
		
		offscreen:
		# ERASE THE MISSILE FROM THE SCREEN
		movl $32, C(%ebx) # ' '
		pushl %ebx
		call draw_missile
		addl $4, %esp
		# remove the missile and free it
		pushl %ebx
		call mp1_remove_missile
		addl $4, %esp
		movl %eax, %ebx
		cmpl $0, %ebx
		je zero_missiles
		jmp next_missile

		zero_missiles:
		movl %edi, %eax
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

# function draws missiles given whatever c field character is given 
# Arg: missile : the missiles pointer to draw
# returns : nothing
draw_missile:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# access the arg and calculate text-mode position
		movl 8(%ebp), %ebx
		movl $0xFFFF0000, %esi
		andl X(%ebx), %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		shr %esi
		movl $0xFFFF0000, %edi
		andl Y(%ebx), %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
		shr %edi
	# %esi = x position, %ecx = y position
		imul $160, %edi # 2bytes/char * 80 rows = 160
		imul $2, %esi # 2bytes/char
		addl %edi, %esi
		movl %esi, %eax # %eax has the text-mode position
		movl C(%ebx), %ecx # %ecx(%cl) has the character code in ASCII
		call mp1_poke
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

# function removes a missile from the linked list and frees it from memory
# Arg: Missile : the missiles pointer to be removed
# returns: pointer to the missile next of the removed missile in the list, or null if no missiles are ahead of this missile
mp1_remove_missile:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# remove and free the missile and get the NEXT address
		movl 8(%ebp), %ebx
		movl NEXT(%ebx), %eax
		pushl %eax
		movl mp1_missile_list, %esi
		cmpl %ebx, %esi
		je head_removal
		check_next_missile:
		cmpl NEXT(%esi), %ebx
		je previous_found
		movl NEXT(%esi), %esi
		jmp check_next_missile

		previous_found:
		movl %eax, NEXT(%esi)
		pushl %ebx
		call mp1_free
		addl $4, %esp
		jmp finished_removal

		head_removal:
		movl %eax, mp1_missile_list
		pushl %ebx
		call mp1_free
		addl $4, %esp

		finished_removal:
		popl %eax
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

mp1_ioctl_startgame:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# initializing driver variables
		andl $0, %ecx
		movl %ecx, mp1_score
		movl %ecx, mp1_missile_list
		movl $0x00010101, %edx
		movl %edx, base_alive # I DON"T KNOW WHAT THIS SHOULD BE INITIALIZED TO
		addl $40, %ecx
		movl %ecx, crosshairs_x
		subl $28, %ecx
		movl %ecx, crosshairs_y
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret
	
mp1_ioctl_addmissile:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# add a new missile which is a parameter(arg)
		pushl $36
		call mp1_malloc
		popl %edi
		cmpl $0, %eax
		je bad_exit
		movl %eax, %esi
	# %esi is an allocated struct head for the new missile
		# unsigned long mp1 copy from user (void *to, const void *from, unsigned long n);
		movl 8(%ebp), %ecx 
		pushl $36
		pushl %ecx
		pushl %esi
		call mp1_copy_from_user
		popl %esi # now %esi holds the kernel arg and we can confirm the copy
		addl $8, %esp
		cmpl $0, %eax
		jne bad_exit_2
		# %esi is allocated and a valid copy, time to insert
		movl mp1_missile_list, %edx
		movl %edx, NEXT(%esi)
		movl %esi, mp1_missile_list
		andl $0, %eax
		jmp exit
		
		bad_exit_2:
		pushl %esi
		call mp1_free
		# popl %esi
		addl $4, %esp
		bad_exit:
		andl $0, %eax
		subl $1, %eax
	# calling convetion register teardown
		exit:
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

mp1_ioctl_movexhairs:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# removed the old crosshair
		movl crosshairs_y, %ebx
		movl crosshairs_x, %eax
		imull $160, %ebx
		imull $2, %eax
		addl %ebx, %eax
		movl $32, %ecx
		call mp1_poke
	# breakdown the argument separate x change and y change(THIS MIGHT BE USER TO KERNEL)
		movl 8(%ebp), %ebx
		movl %ebx, %esi
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		shl %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %ebx
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
		sar %esi
	# %ebx = xchange %esi = ychange so now we bound test to move our position
		movl crosshairs_x, %edi
		addl %ebx, %edi
		cmpl $79, %edi
		jg above_bounds_x
		cmpl $0, %edi
		jl below_bounds_x
		jmp in_bounds_x
		y_stuff:
		andl $0, %ecx
		addb $43, %cl # should be a plus sign
		movl crosshairs_y, %edi
		addl %esi, %edi
		cmpl $24, %edi
		jg above_bounds_y
		cmpl $0, %edi
		jl below_bounds_y
		jmp in_bounds_y
	# whether or not the position is bound is known, so we can adjust the crosshair accordingly
		above_bounds_x:
		movl $79, crosshairs_x
		andl $0, %ebx
		addl $158, %ebx # 79*2bytes per char
		jmp y_stuff
		below_bounds_x:
		movl $0, crosshairs_x
		andl $0, %ebx
		jmp y_stuff
		in_bounds_x:
		movl %edi, crosshairs_x
		movl %edi, %ebx
		imull $2, %ebx
		jmp y_stuff

		above_bounds_y:
		movl $24, crosshairs_y
		andl $0, %eax
		addl $3840, %eax # 24*2bytes per char*80chars per line
		addl %ebx, %eax
		call mp1_poke
		# call poke function with the correct crosshair character (%cl is supposed to be the char)
		jmp crosshairs_set
		below_bounds_y:
		movl $0, crosshairs_y
		andl $0, %eax
		addl %ebx, %eax
		call mp1_poke
		# call poke function with the correct crosshair character (%cl is supposed to be the char)
		jmp crosshairs_set
		in_bounds_y:
		movl %edi, crosshairs_y
		movl %edi, %eax
		imull $160, %eax # 80cols*2byte/char
		addl %ebx, %eax
		call mp1_poke
		# call poke function with the correct crosshair character (%cl is supposed to be the char)
		# jmp crosshairs_set
		crosshairs_set:
		andl $0, %eax
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

mp1_ioctl_getstatus:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# 32-bit arg [31:19] = 0, [18:16] city status, [15:0] current score
		movl mp1_score, %ebx
		movl base_alive, %esi # with all cities alive... 01010100
		movl $0x00000001, %edi
		andl %esi, %edi
		jnz city_one_alive
		city_two:
		movl $0x00000100, %edi
		andl %esi, %edi
		jnz city_two_alive
		city_three:
		movl $0x00010000, %edi
		andl %esi, %edi
		jnz city_three_alive
		jmp status_ready

		city_one_alive:
		movl $0x00010000, %edi
		orl %edi, %ebx
		jmp city_two
		city_two_alive:
		movl $0x00020000, %edi
		orl %edi, %ebx
		jmp city_three
		city_three_alive:
		movl $0x00040000, %edi
		orl %edi, %ebx

		status_ready: # ebx should have the right status data
		# allocate memory for the status value
		pushl $4
		call mp1_malloc
		addl $4, %esp
		cmpl $0, %eax
		je copy_failed
		movl %ebx, 0(%eax)
		# unsigned long mp1 copy to user (void *to, const void *from, unsigned long n);
		pushl $4
		pushl %eax
		pushl 8(%ebp)
		call mp1_copy_to_user
		addl $12, %esp
		cmpl $0, %eax
		jnz copy_failed
		jmp exit_status

		copy_failed:
		andl $0, %eax
		addl $-1, %eax
		jmp exit_status

		exit_status:
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret

mp1_ioctl_endgame:
	# push stack frames base pointer
		pushl %ebp
		movl %esp, %ebp
	# calling convention register saves
		pushl %ebx
		pushl %esi 
		pushl %edi
	# mp1_missile_list is the head to the list, so traverse to tail while pushing to save previous
		movl mp1_missile_list, %ebx
		andl $0, %esi
		cmpl $0, %ebx
		jz no_missiles
		next_again:
		cmpl $0, NEXT(%ebx)
		je at_tail
		pushl %ebx
		addl $1, %esi # counts all the pushes to the stack
		movl NEXT(%ebx), %ebx
		jmp next_again

		at_tail: # %ebx is tail and stack has the previous missile(s)
		pushl %ebx
		freeing:
		call mp1_free
		addl $4, %esp
		addl $-1, %esi
		cmpl $0, %esi
		jl no_missiles
		jmp freeing

		no_missiles:
		andl $0, %eax
	# calling convetion register teardown
		popl %edi
		popl %esi
		popl %ebx
	# teardown locals and leave
		leave
		ret
