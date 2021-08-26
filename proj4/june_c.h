// Register
#define zero    0
#define at      1
#define v0      2
#define v1      3
#define a0      4
#define a1      5
#define a2      6
#define a3      7
#define t0      8
#define t1      9
#define t2      10
#define t3      11
#define t4      12
#define t5      13
#define t6      14
#define t7      15
#define s0      16
#define s1      17
#define s2      18
#define s3      19
#define s4      20
#define s5      21
#define s6      22
#define s7      23
#define t8      24
#define t9      25
#define k0      26
#define k1      27
#define gp      28
#define sp      29
#define fp      30
#define ra      31

// R
#define ADD 0x20
#define ADDU 0x21
#define AND 0x24
#define JR 0x08
#define JALR 0x09
#define NOR 0x27
#define OR 0x25
#define SLT 0x2a
#define SLTU 0x2b
#define SLL 0x00
#define SRL 0x02
#define SUB 0x22
#define SUBU 0x23

// I
#define ADDI 0x08
#define ADDIU 0x09
#define ANDI 0x0c
#define BEQ 0x04
#define BNE 0x05
#define LBU 0x24
#define LHU 0x25
#define LL 0x30
#define LUI 0x0f
#define LW 0x23
#define ORI 0x0d
#define SLTI 0x0a
#define SLTIU 0x0b
#define SB 0x28
#define SC 0x38
#define SH 0x29
#define SW 0x2b

// J
#define J 0x02
#define JAL 0x03

typedef struct ifid{
	int instruction;
	int PC;
}IFID;

typedef struct idex{
	unsigned int opcode, rs, rd, rt, imm, addr, shamt, func, instruction;
    int PC;
	int JumpAddr,BranchAddr;
    int ZeroExtImm,SignExtImm;
	int RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite;
	int val1,val2;
	int temp;
    int ALUResult;
    int index,word;
}IDEX;

typedef struct exmem{
	unsigned int opcode, rs, rd, rt, imm, addr, shamt, func, instruction;
    int PC;
	int JumpAddr, BranchAddr;
    int ZeroExtImm, SignExtImm;
	int RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite;
	int val1, val2;
    int temp;
	int WriteReg, ALUResult, ReadData;
    int index,word;
}EXMEM;

typedef struct memwb{
	unsigned int opcode, rs, rd, rt, imm, addr, shamt, func, instruction;
    int PC;
	int JumpAddr, BranchAddr;
    int ZeroExtImm, SignExtImm;
	int RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite;
	int val1, val2;
    int temp;
	int WriteReg, ALUResult, ReadData;
    int index,word;
}MEMWB;

typedef struct cacheline{
	unsigned int tag;
	//unsigned int sca;
	unsigned int valid;
	unsigned int dirty;
	int data[16];
}CACHELINE;
