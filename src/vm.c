/**
 * Virtual machine for executing binary file from synacor challange
 *
 * https://challenge.synacor.com/
 * 
 * TODO:
 *  - objdump
 *  - strings
 */

/*************************************************************
 * Includes
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************
 * Defines
 */
 
#define STACK_SIZE		32768
#define FMT_VM_TAG		"[vm] "
#define FMT_VM_FAIL		"\n[vm] Aborted!\n"

#define ARCH_MODULO		32768

/**
 * Info from arch_spec file for storage constants
 *   - numbers 0..32767 mean a literal value
 *   - numbers 32768..32775 instead mean registers 0..7
 *   - numbers 32776..65535 are invalid
 */
 
#define REGISTERS_SIZE			8
#define	STORAGE_SIZE			65535
#define STORAGE_MEM_LOW			0
#define STORAGE_MEM_HIGH		32767
#define STORAGE_REG_LOW			32768
#define STORAGE_REG_HIGH		32775
#define STORAGE_INV_LOW			32776
#define MEMORY_SIZE				32767

#define VALUE_MAX_LITERAL		32767
#define VALUE_MAX_REGISTER		32775

/*************************************************************
 * Declarations
 */

/* Registers functions */
void			reg_init		(void);
unsigned short 	reg_read		(unsigned short address);
void 			reg_write		(unsigned short address, unsigned short value);

/* Memory functions */
void			mem_init		(void);
unsigned short 	mem_read		(unsigned short address);
void 			mem_write		(unsigned short address, unsigned short value);

/* Stack functions */
void			stack_init		(void);
int				stack_is_empty	(void);
int				stack_is_full	(void);
unsigned short 	stack_pop		(void);
void		 	stack_push		(unsigned short val);

/* Binary file functions */
int binary_init					(int argc, char *argv[]);
int binary_load					(void);
int binary_exec					(void);

/* CPU operation */
int operation_exec				(unsigned short opcode, unsigned short a, unsigned short b, unsigned short c, int *jmp); 

/* Helper functions */
unsigned short 	val_get			(unsigned short input);

void vm_info					(const char *fmt, ...);
void vm_fail					(const char *fmt, ...);

/*************************************************************
 * Data types
 */
 
typedef struct {
	int 			size;
	int				length;
	char			*path;
} binary_t;

typedef struct {
	int 			position;
	unsigned short 	contents	[STACK_SIZE];
} stack_t;

typedef struct {
	unsigned short 	contents	[MEMORY_SIZE];
} memory_t;

typedef struct {
	unsigned short 	contents	[REGISTERS_SIZE];
} registers_t;

/*************************************************************
 * Global variables
 */
 
/* Binary info */
binary_t 		binary;

/* Storage */
stack_t 		stack;
memory_t 		memory;
registers_t		registers;

/*************************************************************
 * Functions
 */

/**
 * Main program function
 */

int main(int argc, char *argv[])
{
	int ret;
	
	/* Set binary file path */
	ret = binary_init(argc, argv);
	if (ret < 0) {
		vm_fail("Please provide path to binary file ...");
	}
	
	/* Initializes registers, memory and stack */
	reg_init();
	mem_init();
	stack_init();

	/* Load program file */	
	ret = binary_load();
	if (ret < 0) {
		vm_fail("Loading failed ...\n");
	}
	
	/* Execute program */
	ret = binary_exec();
	if (ret < 0) {
		vm_fail("Execution failed ...\n");
	}
	
	return 0;
}

/**
 * Initializes binary file
 */

int binary_init(int argc, char *argv[])
{
	/* Set path to binary file - first argument */
	binary.path = argv[1];
	
	return (argc < 2) ? -1 : 0;
}

/**
 * Loads binary file into memory
 */
 
int binary_load() 
{
	int ret;
	FILE *fp;

	/* Info */
	vm_info("Loading program ...");

	/* Open binary file */
	fp = fopen(binary.path, "r");
	if (!fp) {
		vm_fail("Cannot open binary file ... [%s]", binary.path);
	}
	
	/* Get binary size */
	fseek(fp, 0L, SEEK_END);
	binary.size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	
	/* Info */
	vm_info("Binary path: %s",		binary.path);
	vm_info("Binary size: %d B", 	binary.size);
	
	/* Load binary to memory at offset 0 */
	ret = fread(&memory.contents[0], sizeof(unsigned char), binary.size, fp);
	if (ret != binary.size) {
		vm_fail("Cannot load binary file into memory ...");
	}
	
	/* Set length of binary instructions */
	binary.length = binary.size / 2;
	free(fp);
	return 0;
}

/**
 * Executes binary file written in memory
 */
 
int binary_exec()
{
	/* Program counter */
	int pc = 0;
	vm_info("Executing program ...");

	/* Execute binary program */
	while (1) {
		
		operation_exec(
			memory.contents[pc],
			memory.contents[pc+1],
			memory.contents[pc+2],
			memory.contents[pc+3],
			&pc);
		
		if (pc < 0 || pc > binary.length) {
			vm_fail("Program counter out of bounds.");
		}
	}
	return 0;
}

/**
 * Executes given operation
 */

int operation_exec(unsigned short opcode, unsigned short a, unsigned short b, unsigned short c, int *pc)
{	
	switch (opcode) {
		/* Halt */
		case 0 	:
			vm_info("Halted ... [pc:%d]", *pc);
			exit(0);
		/* Set */
		case 1 	:
			reg_write(a, reg_read(b));
			*pc += 3;
			break;
		/* Push */
		case 2 	:
			stack_push(val_get(a));
			*pc += 2;		
			break;
		/* Pop */
		case 3 	:
			reg_write(a, stack_pop());
			*pc += 2;
			break;
		/* Eq */
		case 4 	:
			reg_write(a, val_get(b) == val_get(c) ? 1 : 0);
			*pc += 4;
			break;
		/* Gt */
		case 5 	:
			reg_write(a, val_get(b) > val_get(c) ? 1 : 0);
			*pc += 4;
			break;
		/* Jmp */
		case 6 	:
			*pc = val_get(a);
			break;
		/* Jt */
		case 7 	:
			*pc = (val_get(a) != 0) ? val_get(b) : *pc + 3;
			break;
		/* Jf */
		case 8 	:
			*pc = (val_get(a) == 0) ? val_get(b) : *pc + 3;
			break;
		/* Add */
		case 9 	:
			reg_write(a, (val_get(b) + val_get(c)) % ARCH_MODULO);
			*pc += 4;
			break;
		/* Mult */
		case 10 :
			reg_write(a, (val_get(b) * val_get(c)) % ARCH_MODULO);
			*pc += 4;
			break;
		/* Mod */
		case 11 :
			reg_write(a, val_get(b) % val_get(c));
			*pc += 4;
			break;		  
		/* And */
		case 12 :
			reg_write(a, val_get(b) & val_get(c));
			*pc += 4;
			break;	
		/* Or */
		case 13 :
			reg_write(a, val_get(b) | val_get(c));
			*pc += 4;
			break;
		/* Not */
		case 14 :
			reg_write(a, (~val_get(b)) & 0x7fff);
			*pc += 3;
			break;
		/* Rmem */
		case 15 :
			reg_write(a, mem_read(val_get(b)));
			*pc += 3;
			break;
		/* Wmem */
		case 16 :
			mem_write(val_get(a), val_get(b));
			*pc += 3;
			break;
		/* Call */
		case 17 :
			stack_push(*pc+2);	/* Push address of next instruction to stack */
			*pc = val_get(a);	
			break;
		/* Ret */
		case 18 :
			*pc = stack_pop();  /* Pop address of next instruction from stack */
			break;
		/* Out */
		case 19 :
			putchar(val_get(a));
			*pc += 2;
			break;
		/* In */
		case 20 :
			reg_write(a, getchar());
			*pc += 2;
			break;
		/* No op */
		case 21 :
			*pc += 1;
			break;
		/* Error */
		default :
			vm_fail("Function %s() failed! [opcode:%d] [pc:%d]", __FUNCTION__, opcode, *pc);
	}
	
	return 0;
}

/**
 * Returns literal value or value stored in register
 */

unsigned short val_get(unsigned short input)
{
	if (input <= VALUE_MAX_LITERAL) {
		return input;
	}
	if (input <= VALUE_MAX_REGISTER) {
		return reg_read(input);
	}
	
	vm_fail("Function %s() failed!", __FUNCTION__);
	return 0;
}

/**
 * Initializes registers
 */
 
void reg_init()
{
	memset(&registers, 0, sizeof(registers_t));
}

/**
 * Writes value to register
 */

void reg_write(unsigned short address, unsigned short value)
{
	if (address < STORAGE_REG_LOW || address > STORAGE_REG_HIGH) {
		vm_fail("Function %s() failed!", __FUNCTION__);
	}
	
	registers.contents[address-STORAGE_REG_LOW] = value;
}

/**
 * Reads form register or returns literal value
 */

unsigned short reg_read(unsigned short address)
{
	if (address < STORAGE_REG_LOW) {
		return address;
	} else {
		return registers.contents[address-STORAGE_REG_LOW];
	}
}

/**
 * Initializes memory
 */
 
void mem_init()
{
	memset(&memory, 0, sizeof(memory_t));
}

/**
 * Reads from memory
 */
 
unsigned short mem_read(unsigned short address)
{
	return memory.contents[address];
}

/**
 * Writes to memory
 */

void mem_write(unsigned short address, unsigned short value)
{
	memory.contents[address] = value;
}

/**
 * Initializes stack
 */
 
void stack_init()
{
	stack.position = -1;
}

/**
 * Pops element from stack
 */

unsigned short stack_pop()
{
	if (stack_is_empty()) {
		vm_fail("Function stack_pop() failed!");
	}
	
	return stack.contents[stack.position--];
}
 
/**
 * Pushes element to stack
 */
 
void stack_push(unsigned short element) 
{
	if (stack_is_full()) {
		vm_fail("Function %s() failed!", __FUNCTION__);	
	}

	stack.contents[++stack.position] = element;
}
	
/**
 * Returns true if stack is full
 */
 
int stack_is_full()
{
	return stack.position >= (STACK_SIZE - 1);
}
/**
 * Returns true if stack is empty
 */
 
int stack_is_empty()
{
	return stack.position < 0;
}
	
/**
 * Prints info to standard error
 */
 
void vm_info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf	(stderr, FMT_VM_TAG);
	vfprintf(stderr, fmt, args);
	fprintf	(stderr, "\n");
	va_end(args);
}

/**
 * Prints info to standard error and exits the program
 */

void vm_fail(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf	(stderr, FMT_VM_TAG);
	vfprintf(stderr, fmt, args);
   	fprintf	(stderr, FMT_VM_FAIL);
	va_end(args);
     
	/* Exit with error */
	exit(1);
}