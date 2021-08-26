#include<stdio.h>
#include<string.h>
#include"june_c.h"

unsigned int Memory[0x400000];
unsigned int R[32] = {0};
int PC = 0;
int cycle=0;
int inst_count = 0;
int exe_count = 0;
int mem_count = 0;
int reg_count = 0;
int jump_count = 0;
int R_count, I_count, J_count = 0;
int branch_count, not_branch_count = 0;
IFID ifid_latch[2];
IDEX idex_latch[2];
EXMEM exmem_latch[2];
MEMWB memwb_latch[2];
CACHELINE cache[1024]; // For 64KB cache

void init();
void Read_Memory(FILE *file);
void R_type(int func, int val1, int val2);
void I_type_exb(int opcode, int val1, int val2);
void J_type(int opcode);
void Branch_Prediction(int opcode, int val1, int val2);
void Update_latch();
int Control_Signal(int opcode);
int Sign_Extend(int imm);
int jump_Addr();
int branch_Addr();
int ReadMem(int addr);
int WriteMem(int addr, int val);
int IF();
int ID();
int EX();
int MEM();
int WB();

int compulsory_count=0;
int hit_count=0;
int miss_count=0;
int readmemorycount=0;
int writememorycount=0;
double hit_ratio=0;
double miss_ratio=0;

int main(){
    FILE *file;
    int ret = 0;

    char *filename;
    filename = "input4.bin";

    file = fopen(filename, "r");
    if (file == NULL){
        printf("Error: There is no file\n");
        return 0;
    }

	Read_Memory(file);
	fclose(file);

    init();

	while(1){
		ret = IF();	
		if(ret <= 0) break;
		ret = ID();
		if(ret <= 0) break;
		ret = EX();	
		if(ret <= 0) break;	
		ret = MEM();
		if(ret <= 0) break;
		ret = WB();
		if(ret <= 0) break;

		Update_latch();

		//printf("cycle: %d\n",cycle);
		cycle++;
		inst_count++;
	}

    hit_ratio = (double)(hit_count)/(hit_count+miss_count)*100;
	miss_ratio = (double)(miss_count)/(hit_count + miss_count)*100;

    printf("[FINAL RESULT]----------------------------------------\n");
	printf("> Filename : %s\n",filename);
    printf("> Cycle : %d\n",cycle-1);
	printf("> R[v0]: %d\n", R[v0]);
	printf("> R-type[%d]\tI-type[%d]\tJ-type[%d]\n",R_count,I_count,J_count);
	printf("> Instruction Count: %d\n", inst_count-1);
    printf("> Instruction_Execution Count: %d\n",exe_count);
	printf("> Memory Update Count: %d\n",mem_count);
	printf("> Register Update Count: %d\n",reg_count);
	printf("> Branch Count: %d\n",branch_count);
	printf("> Not Branch Count: %d\n",not_branch_count);
	printf("> Total Branch op: %d\n",branch_count+not_branch_count);
	printf("> Jump Count: %d\n",jump_count);
    printf("> Hit Ratio : %f\n",hit_ratio);
    printf("> Miss Ratio : %f\n",miss_ratio);
	printf("------------------------------------------------------\n");

	return 0;
}

int ReadMem(int addr){
	int ret;

	int tag = (addr & 0xFFFF0000) >> 16;
	int index = (addr & 0x0000FFC0) >> 6;
	int offset = addr & 0x0000003F;
	int destination_addr = (addr & 0xFFFFFFC0);
	int drain_addr = (cache[index].tag << 16) | (index << 6);

	if(cache[index].valid == 1){	
		if (cache[index].tag == tag){
			hit_count++;
			return cache[index].data[offset/4];
		} else if (cache[index].tag != tag){
			if(cache[index].dirty == 1){
				for(int i = 0; i < 16; i++){
					Memory[(drain_addr/4)+i] = cache[index].data[i];
				}
			}
		
			for (int i=0; i<16; i++){	
				cache[index].data[i]=Memory[(destination_addr/4)+i];
			}

			cache[index].tag=tag;
			cache[index].valid=1;
			cache[index].dirty=0;
			miss_count++;

			return cache[index].data[offset/4];
		} 
	} else if(cache[index].valid != 1){
		for (int i = 0; i < 16; i++){
			cache[index].data[i]=Memory[(destination_addr/4)+i];
		}

		cache[index].tag = tag;
		cache[index].valid = 1;
		cache[index].dirty = 0;
		miss_count++;
		compulsory_count++;

		return cache[index].data[offset/4];
	}
	return ret;
}

int WriteMem(int addr, int val){
	int ret = 1;

	int tag = (addr & 0xFFFF0000) >>16;
	int index = (addr & 0x0000FFC0) >> 6;
	int offset = addr & 0x0000003F;
	int destination_addr = (addr &0xFFFFFFC0);
	int drain_addr = (cache[index].tag >> 16) | (index >> 6);

	if(cache[index].valid == 1){
		if(cache[index].tag != tag){
			if(cache[index].dirty ==1){
				for(int i=0;i<16;i++){
					Memory[(drain_addr/4)+i]=cache[index].data[i];
				}
			}
		
			for(int i=0; i<16; i++){
				cache[index].data[i]=Memory[(destination_addr/4)+i];
			}

			cache[index].data[offset/4]= val;
			cache[index].tag = tag;
			cache[index].dirty = 1;
			cache[index].valid = 1;
			hit_count++;		
			return 0;
		} else if (cache[index].tag == tag){
			cache[index].data[offset/4] = val;
			cache[index].dirty=1;
			return 0;
		}
	} else if(cache[index].valid != 1){
		for (int i=0; i<16; i++){
			cache[index].data[i]=Memory[(destination_addr/4)+i];
		}

		cache[index].data[offset/4] = val;
		cache[index].tag=tag;
		cache[index].valid=1;
		cache[index].dirty=1;
		//printf("cold miss\n");
		miss_count++;
		return 0;
	}

	
	return ret;
}

void init(){
    R[ra] = 0xFFFFFFFF;
    R[sp] = 0x100000;
}

void Read_Memory(FILE *file){
    unsigned int ret = 0;
    int i = 0;
    int memVal;
    while(1){
        int data = 0;
        ret = fread(&data, 1, sizeof(int), file);
        memVal = ntohl(data);
        Memory[i] = memVal;
        //printf("(Load) RET[%d] M[%X] Pointer Address: 0x%X Value: 0x%08X\n", (int)ret, i*4, &Memory[i], Memory[i]);
        i++;
        if(ret != 4){
            break;
        }
    }
}

int IF(){
	int ret = 1;
	if (PC == 0xFFFFFFFF) return 0;
	//ifid_latch[0].instruction = Memory[PC/4];
    ifid_latch[0].instruction = ReadMem(PC);
	ifid_latch[0].PC = PC;
	// Update PC
	PC = PC + 4;
	//printf("[Fetch]\tPC: %08X\tInstruction: %08X\n", PC, ifid_latch[0].instruction);
	return ret;
}

int Control_Signal(int opcode){
	int ret = 1;

    // MUX
	if(opcode == 0x0){
        idex_latch[0].RegDst = 1;
    } else {
        idex_latch[0].RegDst = 0;
    }

    if((opcode == J) || (opcode == JAL)) {
        idex_latch[0].Jump = 1;
    } else {
        idex_latch[0].Jump = 0;
    }

    if((opcode == BEQ) || (opcode == BNE)) {
        idex_latch[0].Branch = 1;
    } else {
        idex_latch[0].Branch = 0;
    }

    if(opcode == LW) {
        idex_latch[0].MemtoReg = 1;
        idex_latch[0].MemRead = 1;
    } else {
        idex_latch[0].MemtoReg = 0; 
        idex_latch[0].MemRead = 0;
    }

    if(opcode == SW) {
        idex_latch[0].MemWrite = 1;
    } else {
        idex_latch[0].MemWrite = 0;
    }

	if((opcode != 0) && (opcode != BEQ) && (opcode != BNE)){
        idex_latch[0].ALUSrc=1;
    } else {
        idex_latch[0].ALUSrc=0;
    }

	if((opcode != SW) && (opcode != BEQ)&& (opcode != BNE) && (opcode != JR) && (opcode != J) && (opcode != JAL)) {
        idex_latch[0].RegWrite = 1;
    } else {
        idex_latch[0].RegWrite = 0;
    }	

    //printf("RegDst :%d\tJump: %d\tBranch: %d\tMemRead: %d\tMemtoReg: %d\tMemWrite: %d\tALUSrc: %d\tRegWrite: %d\n",idex_latch[0].RegDst, idex_latch[0].Jump, idex_latch[0].Branch, idex_latch[0].MemRead, idex_latch[0].MemtoReg, idex_latch[0].MemWrite, idex_latch[0].ALUSrc, idex_latch[0].RegWrite);
	return ret;
}

int Sign_Extend(int imm){
	int SignExtImm;
    int SignBit = (imm & 0x0000FFFF) >> 15;
    if(SignBit == 1){
        SignExtImm = 0xFFFF0000 | imm;
    } else{
        SignExtImm = 0x0000FFFF & imm;
    }
	return SignExtImm;
}

int ID(){
	int ret = 1;
	int opcode;

    idex_latch[0].opcode = ((ifid_latch[1].instruction & 0xFC000000));
    idex_latch[0].opcode = (idex_latch[0].opcode >> 26) & 0x3F;
	opcode = idex_latch[0].opcode;

    idex_latch[0].rs = (ifid_latch[1].instruction & 0x03E00000);
    idex_latch[0].rs = (idex_latch[0].rs >> 21) & 0x1F;

    idex_latch[0].rt = (ifid_latch[1].instruction & 0x001F0000);
    idex_latch[0].rt = (idex_latch[0].rt >> 16) & 0x1F;

    idex_latch[0].rd = (ifid_latch[1].instruction & 0x0000F800);
    idex_latch[0].rd = (idex_latch[0].rd >> 11) & 0x1F;

    idex_latch[0].shamt = (ifid_latch[1].instruction & 0x000007C0);
    idex_latch[0].shamt = idex_latch[0].shamt >> 6;

    idex_latch[0].func = (ifid_latch[1].instruction & 0x0000003F);

    idex_latch[0].imm = (ifid_latch[1].instruction & 0x0000FFFF);

    idex_latch[0].addr = (ifid_latch[1].instruction & 0x03FFFFFF);

	switch(opcode){
		case 0:
		break;

		case J: 
			J_count++;
		break;

		case JAL:
			J_count++;
		break;

		default : 
			I_count++;
		break;
	}


	// sign extend
	int imm = ifid_latch[1].instruction;
	idex_latch[0].SignExtImm = Sign_Extend(imm);

	// Update latch
	idex_latch[0].instruction = ifid_latch[1].instruction;
	idex_latch[0].PC = ifid_latch[1].PC;

    J_type(opcode);
	Control_Signal(opcode);

	return ret;
}

int jump_Addr(){
	idex_latch[0].JumpAddr = (ifid_latch[1].PC & 0xF0000000) | (idex_latch[0].addr <<2);
	int j = idex_latch[0].JumpAddr;
	return j;
}

int branch_Addr(){
	idex_latch[1].BranchAddr = idex_latch[1].SignExtImm << 2;
	int b = idex_latch[1].BranchAddr;
	return b;
}

void J_type(int opcode){
    if(opcode == J){
		PC = jump_Addr();
		exe_count++;
		jump_count++;
	} else if (opcode == JAL){	
		R[ra] = idex_latch[0].PC+8;
		PC = jump_Addr();
		exe_count++;
		jump_count++;
	}
}

void R_type(int func,int val1, int val2){
    switch(func){
	case ADD:
	//printf("ADD\n");
	exmem_latch[0].ALUResult = val1 + val2;
	exe_count++;
	break;

	case ADDU:
	//printf("ADDU\n");
	exmem_latch[0].ALUResult = val1 + val2;
	exe_count++;
	break;

    case AND: 
	//printf("AND\n");
	exmem_latch[0].ALUResult = val1 & val2;
	exe_count++;
	break;

	case JR:
	//printf("JR\n");
	PC=val1;
	memset(&ifid_latch[0], 0, sizeof(IFID));
	//printf("opcode: %d, PC: %d\n",func,val1);
	break;

	case JALR:
	//printf("JALR\n");
	PC=val1;
	memset(&ifid_latch[0], 0, sizeof(IFID));
	memset(&idex_latch[0], 0, sizeof(IDEX));
	//printf("opcode: %d, PC: %d\n",func,val1);
	break;

    case NOR:
	//printf("NOR\n");
	exmem_latch[0].ALUResult = !(val1|val2);
	exe_count++;
	break;

	case OR:
	//printf("OR\n");
	exmem_latch[0].ALUResult = (val1|val2);
	exe_count++;
	break;

	case SLT:
	//printf("SLT\n");
	exmem_latch[0].ALUResult = ((val1<val2) ? 1:0);
	exe_count++;
	break;

	case SLTU:
	//printf("SLTU\n");
	exmem_latch[0].ALUResult = ((val1<val2) ? 1:0);
	exe_count++;
	break;

	case SLL:
	//printf("SLL\n");
	exmem_latch[0].ALUResult = val2 << idex_latch[1].shamt;	
	exe_count++;
	break;

    case SRL:
	//printf("SRL\n");
	exmem_latch[0].ALUResult = val2 >> idex_latch[1].shamt;
	exe_count++;
	break;

	case SUB:
	//printf("SUB\n");
	exmem_latch[0].ALUResult = val1 - val2;
	exe_count++;
	break;

    case SUBU:
	//printf("SUBU\n");
    exmem_latch[0].ALUResult = val1 - val2;
	exe_count++;
	break;

	default:
	break;
	}
}

void I_type_exb(int opcode, int val1, int val2){
    int temp;
    switch(opcode){
	case ADDI:
	//printf("ADDI\n");
	exmem_latch[0].ALUResult = val1 + idex_latch[1].SignExtImm;
	exe_count++;
	break;

    case ADDIU:
	//printf("ADDIU\n");
	exmem_latch[0].ALUResult = val1 + idex_latch[1].SignExtImm;
	exe_count++;
	break;

	case ANDI:
	//printf("ANDI\n");
	exmem_latch[0].ALUResult = val1 & idex_latch[1].ZeroExtImm;
	exe_count++;
	break;

    case LBU:
    exmem_latch[0].ALUResult = idex_latch[1].rs + idex_latch[1].SignExtImm & 0x000000FF;
    exmem_latch[0].ALUResult = (Memory[exmem_latch[0].ALUResult/4]) & 0x000000FF;
    exe_count++;
    break;

    case LHU:
    exmem_latch[0].ALUResult = idex_latch[1].rs + idex_latch[1].SignExtImm & 0x000000FF;
    exmem_latch[0].ALUResult = (Memory[exmem_latch[0].ALUResult/4]) & 0x0000FFFF;
    break;  

    case LL:
    exmem_latch[0].rt = Memory[idex_latch[1].rs + idex_latch[1].SignExtImm/4];
    break;

    case LUI:
	exmem_latch[0].ALUResult = idex_latch[1].imm << 16;
	exe_count++;
	break;

    case LW:
	//printf("LW\n");
	exmem_latch[0].temp = val1 + idex_latch[1].SignExtImm;
	//printf("opcode: %d, result: %d\n",opcode,exmem_latch[0].temp);
	exe_count++;
	break;

    case ORI:
	exmem_latch[0].ALUResult = val1 | idex_latch[1].ZeroExtImm;
	exe_count++;
	break;

	case SLTI:
	exmem_latch[0].ALUResult = (val1 < (idex_latch[1].SignExtImm) ? 1:0);
	exe_count++;
	break;

    case SLTIU:
	exmem_latch[0].ALUResult =((val1 < idex_latch[1].SignExtImm)? 1:0);
	exe_count++;
	break;

    case SB:
    exmem_latch[0].ALUResult = idex_latch[1].rs + idex_latch[1].SignExtImm;
    temp= Memory[exmem_latch[0].ALUResult/4];
    temp= temp & 0xFFFFFF00;
    temp= (idex_latch[1].rt & 0x000000FF)|temp;
    Memory[exmem_latch[0].ALUResult/4] = temp;
    break;

    case SC:
    exmem_latch[0].ALUResult = idex_latch[1].SignExtImm;
    temp = idex_latch[1].rs + exmem_latch[0].ALUResult;
    exmem_latch[0].ALUResult = idex_latch[1].rt;
    Memory[temp/4] = exmem_latch[0].ALUResult;
    exmem_latch[0].rt = 1; // R[rt] = (atomic) ? 1:0
    break; 

    case SH:
    exmem_latch[0].ALUResult = idex_latch[1].rs + idex_latch[1].SignExtImm;
    temp = Memory[exmem_latch[0].ALUResult/4];
    temp = temp & 0xFFFF0000;
    temp = (idex_latch[1].rt & 0x0000FFFF) | temp;
    Memory[exmem_latch[0].ALUResult/4] = temp;
    break; 

	case SW:
	//printf("SW\n");
	exmem_latch[0].ALUResult = val2;
	exmem_latch[0].index = val1 + idex_latch[1].SignExtImm;
	//printf("opcode: %d, exmem_latch[0].index: %d\n",opcode,exmem_latch[0].index);
	exe_count++;
	break;

	default:
	break;
    }
}

void Branch_Prediction(int opcode, int val1, int val2){
	// branch not taken
    switch(opcode){
	case BEQ:
	if(val1 == val2){
		int b = branch_Addr();
		PC = idex_latch[1].PC + b + 4;
		//flush
		memset(&ifid_latch[0],0,sizeof(IFID));
		memset(&idex_latch[0],0,sizeof(IDEX));		
		exe_count++;
		branch_count++;
	} else {
        exe_count++;
		not_branch_count++;
	}
	break;
		
    case BNE:
	if(val1 != val2){
		int b = branch_Addr();
		PC = idex_latch[1].PC + b + 4;
		//flush
		memset(&ifid_latch[0],0,sizeof(IFID));
		memset(&idex_latch[0],0,sizeof(IDEX));
        exe_count++;
		branch_count++;
    } else {
		exe_count++;
		not_branch_count++;
	}
	break;
		
    default:
	break;
	}
}

int EX(){
	int ret = 1;
	int val1,val2;

	idex_latch[0].val1 = R[idex_latch[0].rs];
    idex_latch[0].val2 = R[idex_latch[0].rt];
    
	idex_latch[1].ZeroExtImm = (idex_latch[1].instruction & 0x0000FFFF);

	// data forwarding
	if((idex_latch[1].rs != 0) && (idex_latch[1].rs == exmem_latch[0].WriteReg) && (exmem_latch[0].RegWrite)) {
		idex_latch[1].val1  = exmem_latch[0].ALUResult;
	} else if((idex_latch[1].rs != 0) && (idex_latch[1].rs == memwb_latch[0].WriteReg) && (memwb_latch[0].RegWrite)){	
		if(memwb_latch[0].MemtoReg==1){
			idex_latch[1].val1 = memwb_latch[0].ReadData;
		} else {
			idex_latch[1].val1 = memwb_latch[0].ALUResult; 
		}
	}
	if((idex_latch[1].rt != 0) && (idex_latch[1].rt == exmem_latch[0].WriteReg) && (exmem_latch[0].RegWrite)){
		idex_latch[1].val2 = exmem_latch[0].ALUResult;
	} else if((idex_latch[1].rt!=0) && (idex_latch[1].rt == memwb_latch[0].WriteReg) && (memwb_latch[0].RegWrite)){
		if(memwb_latch[1].MemtoReg == 1){
			idex_latch[1].val2 = memwb_latch[0].ReadData;
		} else {
			idex_latch[1].val2 = memwb_latch[0].ALUResult;
		}
	}

	val1 = idex_latch[1].val1;
	val2 = idex_latch[1].val2;

	if(idex_latch[1].RegDst == 1){
        R_type(idex_latch[1].func,val1,val2);
		R_count++;
	} else if (idex_latch[1].ALUSrc==1){
        I_type_exb(idex_latch[1].opcode,val1,val2);
	} else if (idex_latch[1].Branch==1){
        Branch_Prediction(idex_latch[1].opcode,val1,val2);
		//printf("opcode: %d, PC: %d\n",idex_latch[1].opcode,PC);
    }

	/*
	if(idex_latch[0].RegWrite == 1 && idex_latch[0].opcode != LW){
		printf("opcode: %d, ALU Result: %d\n",idex_latch[1].opcode,exmem_latch[0].ALUResult);
	}*/


	if(idex_latch[1].RegDst == 1){ // R-type
		exmem_latch[0].WriteReg = idex_latch[1].rd;
	} else {
		exmem_latch[0].WriteReg = idex_latch[1].rt;
	}

	// Update Latch : exmem_latch[0] = idex_latch[1]
	exmem_latch[0].opcode = idex_latch[1].opcode;
	exmem_latch[0].func = idex_latch[1].func;
	exmem_latch[0].instruction = idex_latch[1].instruction;
    exmem_latch[0].PC = idex_latch[1].PC;
	exmem_latch[0].Jump = idex_latch[1].Jump;
	exmem_latch[0].Branch = idex_latch[1].Branch;
	exmem_latch[0].MemRead = idex_latch[1].MemRead;
	exmem_latch[0].MemtoReg = idex_latch[1].MemtoReg;
	exmem_latch[0].MemWrite = idex_latch[1].MemWrite;
	exmem_latch[0].RegWrite = idex_latch[1].RegWrite;
	//exmem_latch[0].val1 = val1;
	exmem_latch[0].val2 = val2;

	return ret;
}

int MEM(){
	int ret = 1;

    if(exmem_latch[1].MemRead==1){ // LW
		//exmem_latch[1].ReadData=Memory[exmem_latch[1].temp/4];
        memwb_latch[0].ReadData = ReadMem(exmem_latch[1].temp);
		mem_count++;
	} else if (exmem_latch[1].MemWrite==1){ // SW
		//Memory[exmem_latch[1].temp/4]=exmem_latch[1].val2;
        WriteMem(exmem_latch[1].index, exmem_latch[1].ALUResult);
		mem_count++;
	}
	
	// Update Latch : memwb_latch[0] = exmem_latch[1]
	memwb_latch[0].opcode = exmem_latch[1].opcode;
	memwb_latch[0].func = exmem_latch[1].func;
	memwb_latch[0].instruction = exmem_latch[1].instruction;
	memwb_latch[0].MemtoReg = exmem_latch[1].MemtoReg;
	memwb_latch[0].RegWrite = exmem_latch[1].RegWrite;
	memwb_latch[0].val1 = exmem_latch[1].val1;
	memwb_latch[0].val2 = exmem_latch[1].val2;
	memwb_latch[0].ALUResult = exmem_latch[1].ALUResult;
	memwb_latch[0].WriteReg = exmem_latch[1].WriteReg;
	//memwb_latch[0].ReadData = exmem_latch[1].ReadData;

	return ret;
}

int WB(){
	int ret = 1;

	if(memwb_latch[0].RegWrite ==1 ){
		if(memwb_latch[0].MemtoReg == 1){
			R[memwb_latch[0].WriteReg] = memwb_latch[0].ReadData;
		} else{
			if(memwb_latch[0].WriteReg != 0){
				R[memwb_latch[0].WriteReg] = memwb_latch[0].ALUResult;
            } else {
                R[memwb_latch[0].WriteReg] = 0;
            }
		}
		reg_count++;
	}

	return ret;
}

void Update_latch(){
	ifid_latch[1] = ifid_latch[0];
	idex_latch[1] = idex_latch[0];
	exmem_latch[1] = exmem_latch[0];
	memwb_latch[1] = memwb_latch[0];
}