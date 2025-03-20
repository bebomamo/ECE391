#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "file_system_driver.h"
#include "devices/rtc.h"
#include "terminal.h"
#include "syscall.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/*
 *   test_divide_error
 *   DESCRIPTION: Attempt to divide by 0
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Causes a divide by 0 error
 */ 
int test_divide_error() {
	TEST_HEADER;
	int a, b, c;
	a = 10;
	b = 0;
	c = a / b;
	return FAIL;
}

/*
 *   test_bound_range_exceeded
 *   DESCRIPTION: Attempt to access an array out of bounds
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Causes a bound range exceeded error
 */ 
int test_bound_range_exceeded() {
	TEST_HEADER;
	int a[2];
	a[3] = 10;
	return FAIL;
}

/*
 *   test_syscall_handler
 *   DESCRIPTION: Call a system call to see if it triggers interrupt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: FAIL if it doesn't trigger interrupt
 *   SIDE EFFECTS: Causes a system call interrupt
 */ 
int test_syscall_handler() {
	TEST_HEADER;
	asm volatile("int $0x80");
	return FAIL;
}

/*
 *   test_paging
 *   DESCRIPTION: Testing to see if we can access various points in memory
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can access all points in memory
 *   SIDE EFFECTS: may cause page fault if initialization failed
 */
int test_paging() {
	TEST_HEADER;
	uint8_t a;
	uint8_t* p;

	p = (uint8_t *) 0x400000; // point to beginning of kernel memory
    printf("Point to beginning of kernel memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

    p = (uint8_t *) 0x600000; // point to beginning of kernel memory
	printf("Point to middle of kernel memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

    p = (uint8_t *) 0x7FFFFF; // point to end of kernel memory
    printf("Pointing to end of kernel memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

    p = (uint8_t *) 0xB8000; // point to beginning of video memory
    printf("Pointing to beginning of video memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

    p = (uint8_t *) 0xB8FFF; // point to end of vide memory
    printf("Pointing to end of video memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

	return PASS;
}

/*
 *   test_page_fault_handler
 *   DESCRIPTION: Testing to see if we page fault when we dereference NULL
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: FAIL if it doesn't trigger interrupt
 *   SIDE EFFECTS: Causes page fault interrupt
 */
int test_page_fault_handler() {
	TEST_HEADER;
	uint8_t a;
	uint8_t* p;

    p = (uint8_t *) 0x400000; // point to kernel memory
    printf("Point to beginning of kernel memory: 0x%x.\n", p);
    a = *p;
    printf("PASSED\n");

	p = (uint8_t *) 0x2;
    printf("Pointing to not present memory: 0x%x.\n", p);
    a = *p;
    printf("FAILED\n");

	return FAIL;
}

/*
 *   test_null
 *   DESCRIPTION: Testing to see if we page fault when we dereference NULL
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: FAIL if it doesn't trigger interrupt
 *   SIDE EFFECTS: Will print out error message and interrupt should occur
 */
int test_null() {
	TEST_HEADER;
	uint8_t a;
    uint8_t* p;

    p = (uint8_t *) 0;
	printf("Pointing to NULL: 0x%x.\n", p);
	a = *p;

	return FAIL;
}

/* Checkpoint 2 tests */
/*
 *   test_frame1
 *   DESCRIPTION: Testing to see if we can read frame1.txt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read frame1.txt, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_frame1() {
	TEST_HEADER;
	int i;
	init_file_system();

	printf("Reading frame1.txt.\n");
	if (file_open((const uint8_t *) "frame1.txt") == -1) return FAIL;

	char file_buffer[FRAME1_SIZE];
	if (file_read(0, (void *) file_buffer, FRAME1_SIZE) == -1) return FAIL;
	for (i = 0; i < FRAME1_SIZE; i++) {
		putc(file_buffer[i]);
	}

	putc('\n');
	// close and check that it got closed
	if (file_close(0) == -1) return FAIL;
	printf("PASSED\n");

	printf("Attempted to read closed frame1.txt.\n");
	if (file_read(0, (void *) file_buffer, FRAME1_SIZE) == 0) return FAIL;
	printf("PASSED\n");


	return PASS;
}

/*
 *   test_hello
 *   DESCRIPTION: Testing to see if we can read hello executable
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read hello executable, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_hello() {
	TEST_HEADER;
	int i;
	init_file_system();

	printf("Reading hello.\n");
	if (file_open((const uint8_t *) "hello") == -1) return FAIL;

	clear();
	char file_buffer[HELLO_SIZE];
	if (file_read(0, (void *) file_buffer, HELLO_SIZE) == -1) return FAIL;
	for (i = 0; i < HELLO_SIZE; i++) {
		if (file_buffer[i] == '\0') {
			continue;
		}

		putc(file_buffer[i]);
	}

	putc('\n');
	// close and check that it got closed
	if (file_close(0) == -1) return FAIL;
	printf("PASSED\n");

	printf("Attempted to read closed hello executable.\n");
	if (file_read(0, (void *) file_buffer, HELLO_SIZE) == 0) return FAIL;
	printf("PASSED\n");


	return PASS;
}

/*
 *   test_verylarge
 *   DESCRIPTION: Testing to see if we can read verylargetextwithverylongname.txt
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read verylargetextwithverylongname.txt, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_verylarge() {
	TEST_HEADER;
	int i;
	init_file_system();

	printf("Reading verylargetextwithverylongname.txt.\n");
	if (file_open((const uint8_t *) "verylargetextwithverylongname.tx") == -1) return FAIL;

	char file_buffer[VERYLARGE_SIZE];
	if (file_read(0, (void *) file_buffer, VERYLARGE_SIZE) == -1) return FAIL;
	for (i = 0; i < VERYLARGE_SIZE; i++){
		putc(file_buffer[i]);
	}

	putc('\n');
	// close and check that it got closed
	if (file_close(0) == -1) return FAIL;
	printf("PASSED\n");

	printf("Attempted to read closed verylargetextwithverylongname.txt.\n");
	if (file_read(0, (void *) file_buffer, VERYLARGE_SIZE) == 0) return FAIL;
	printf("PASSED\n");

	return PASS;
}

/*
 *   test_directory_ls
 *   DESCRIPTION: Testing to see if we can read directory
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read directory, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_directory_ls() {
	TEST_HEADER;
	int i;
	uint8_t filename[32];
	filename[0] = '.';
	filename[1] = '\0';
	init_file_system();
	if (dir_open(filename) == -1) return FAIL;

	int j;
	char file_buffer[80];

	for (i = 0; i < 17; i++){
		dir_read(0, (void *) file_buffer, 80);
		for (j = 0; j < 80; j++) {
	 		putc(file_buffer[j]);
	 	} 	
	}
	
	dir_close(0);
	putc('\n');
	if (dir_read(0, (void *) file_buffer, 80) == -1) {
		/* should return negative one because we closed the file */
		return PASS;
	} else {
		return FAIL;
	}
}

/*
 *   test_rtc_driver
 *   DESCRIPTION: Testing to see if we can read rtc driver
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read rtc driver, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_rtc_driver() {
	TEST_HEADER;
	int print_repeat = 10;
	int print_counter = 0;
	int wait_ct = 0; 
	int print_char_counter = 0;
	int freq;
	unsigned char blank_buf;

	rtc_open(0);
	
	for (freq = 4; freq <= 256; freq *= 2) {
		rtc_write(0, &freq, 1);
		for(print_counter = 0; print_counter<print_repeat; print_counter++) {
			
			for (wait_ct = 0; wait_ct<freq; wait_ct+=4) {
				rtc_read(0, &blank_buf, 1);
			}
	
			for (print_char_counter = 0; print_char_counter<wait_ct; print_char_counter++) {
				printf("1");
			}		
		}
		
		clear();
	}

	return PASS;
}

/*
 *   test_terminal_driver
 *   DESCRIPTION: Testing to see if we can read terminal driver
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: PASS if we can read terminal driver, FAIL otherwise
 *   SIDE EFFECTS: none
 */
int test_terminal_driver() {
	char buf[128];
	int num_char;
	uint8_t filename[32];
	terminal_open(filename);
	while(1) {
		num_char = terminal_read(0, &buf, 128);
		printf("You typed %d chars: ", num_char);
		terminal_write(0, &buf, num_char);
	}

	return 0;


	// while(1){
	// 	num_char = terminal_read(0, &buf, 128);
	// 	printf("You typed %d chars: ", num_char);
	// 	terminal_write(0, &buf, num_char);
	// }
};

// double cos(double x)
// {
// 	int i;
// 	int n = 10;
//     double sum = 1;
// 	double t = 1;

//     //x=x*3.14159/180;
     
//     /* Loop to calculate the value of Cosine */
//     for(i=1;i<=n;i++)
//     {
//         t=t*(-1)*x*x/(2*i*(2*i-1));
//         sum=sum+t;
//     }
// 	return sum;
// }


// double sin(double x)
// {
	
// 	int i;
// 	int n = 10;
//     double sum = 1;
// 	double t = 1;
	
// 	x += 1.570796;
//     //x=x*3.14159/180;
     
//     /* Loop to calculate the value of Cosine */
//     for(i=1;i<=n;i++)
//     {
//         t=t*(-1)*x*x/(2*i*(2*i-1));
//         sum=sum+t;
//     }
// 	return sum;
// }

/* Checkpoint 3 tests */

int test_sys_calls() {
	// Testing open for our system calls
	// asm volatile("movl $5, %eax");
	// asm volatile("");
	uint8_t buf[128] = "shell";
	int rval;
	// init_file_system();
	// rval = stdin((char *) buf);
	// if(rval == -1) return FAIL;
	// rval = stdout((char *) buf);
	// if(rval == -1) return FAIL;
	// asm volatile ("INT $0x80" : "=a" (rval) : "a" (6), "b" (0));
	// if(rval == -1) return FAIL;
	asm volatile ("INT $0x80" : "=a" (rval) : "a" (2), "b" (buf));
	return PASS;
};

int stdin(char* buf)
{
	uint32_t rval;
	asm volatile ("INT $0x80" : "=a" (rval) : "a" (3), "b" (0), "c" (buf), "d" (6));
	// asm volatile ("pushl $128");
	// asm volatile ("pushl 8(%ebp)");
	// asm volatile ("pushl $0");
	// asm volatile ("int 0x80");
	return rval;
}

int stdout(char* buf)
{
	uint32_t rval;
	asm volatile ("INT $0x80" : "=a" (rval) : "a" (4), "b" (1), "c" (buf), "d" (6));
	// asm volatile ("pushl $128");
	// asm volatile ("pushl 8(%ebp)");
	// asm volatile ("pushl $1");
	// asm volatile ("int 0x80");
	return rval;
}



/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/*
 *   launch_tests
 *   DESCRIPTION: begin of tests
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: performs test indicated by TEST_VECTOR
 */ 
void launch_tests() {
	clear();
	/* Checkpoint 1 Tests */
	// TEST_OUTPUT("General IDT Test", idt_test());
	// TEST_OUTPUT("Divide Error Test", test_divide_error());
	// TEST_OUTPUT("System Call Test", test_syscall_handler());
	// TEST_OUTPUT("Paging Test", test_paging());
	// TEST_OUTPUT("Page Fault Test", test_page_fault_handler());
	// TEST_OUTPUT("NULL Dereference Test", test_null());
	// TEST_OUTPUT("Page fault test", test_page_fault());

	/* Checkpoint 2 Tests */
	// TEST_OUTPUT("Test frame1.txt", test_frame1());
	// TEST_OUTPUT("Test hello executable", test_hello());
	// TEST_OUTPUT("Test verylargetextwithverylongname.txt", test_verylarge());
	// TEST_OUTPUT("Test directory read.", test_directory_ls());
	// TEST_OUTPUT("Testing RTC Driver", test_rtc_driver());
	// TEST_OUTPUT("Testing Terminal Driver", test_terminal_driver());

	/* Checkpoint 3 Tests */
	// TEST_OUTPUT("Test system call", test_sys_calls());
}
