#include<stdio.h>
#include"my.h"
#define u_Size sizeof(unsigned int)

unsigned int opcode, rs, rd, rt, shamt, func, imm, addr, instruction;
unsigned int Memory[0x400000] = {0};
unsigned int R[32] = {0};
unsigned int PC = 0;
int change_pc_val = 0;
int cycle_count = 0;
int R_count = 0;
int I_count = 0;
int J_count = 0;
int memAcc_count = 0;
int branch_count = 0;
int RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite;
int temp,temp2;

void Read_Memory(FILE *file);
void init();
int Instruction_Fetch(FILE *file);
int Instruction_Decode();
int Control_Signal();
int jump_Addr();
int Sign_Extend(int imm);
int branch_Addr(int imm);
int R_Type(int func);
int I_Type(int opcode);
int J_Type(int opcode);
int Exe_and_WB();

int main(){
    FILE *file;
    int ret = 0;

    char *filename;
    filename = "input4.bin";

    file = fopen(filename, "r");
    if (file == NULL){
        printf("Error: There is no file");
        return 0;
    }

    Read_Memory(file);
    fclose(file);

    // initiallize
    init();
    
    // Run program
    while(1){
        ret = Instruction_Fetch(file);
        if(ret <= 0) break;
        ret = Instruction_Decode();
        if(ret <= 0) break;
        ret = Control_Signal();
        if(ret <= 0) break;
        ret = Exe_and_WB();
        if(ret <= 0) break;
        printf("Cycle Count: %d\n",cycle_count);
        printf("\n");
        cycle_count++;
    }

    printf("\nFinal Result\nR[2] = %d\n",R[v0]);
    printf("R-type: %d\tI-type: %d\tJ-type: %d\n",R_count,I_count,J_count);
    printf("Memory Access Inst: %d\tBranch: %d\n",memAcc_count,branch_count);

    return 0;
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

void init(){
    R[ra] = 0xFFFFFFFF;
    R[sp] = 0x1000000;
}

int Instruction_Fetch(FILE *file){
    int ret = 1;
    if (PC == 0xFFFFFFFF) return 0;
    instruction = Memory[PC/4];
    printf("[Fetch]\tPC: %08X\tInstruction: %08X\n", PC, instruction);
    return ret;
}

int Instruction_Decode(){
    int ret = 1;

    opcode = (instruction & 0xFC000000);
    opcode = (opcode >> 26) & 0x3F;
    rs = (instruction & 0x03E00000);
    rs = (rs >> 21) & 0x1F;
    rt = (instruction & 0x001F0000);
    rt = (rt >> 16) & 0x1F;
    rd = (instruction & 0x0000F800);
    rd = (rd >> 11) & 0x1F;
    shamt = (instruction & 0x000007C0);
    shamt = shamt >> 6;
    func = (instruction & 0x0000003F);
    imm = (instruction & 0x0000FFFF);
    addr = (instruction & 0x03FFFFFF);

    printf("----[Decode]----\nopcode[%d], ",opcode);
    switch (opcode){
    case 0:
    printf("rs[%X], rt[%X], rd[%X], shamt[%X], func[%X]\n",rs,rt,rd,shamt,func);
    R_count++;
    break;
    
    case J:
    printf("addr[%X]\n",addr);
    J_count++;
    break;
    case JAL:
    printf("addr[%X]\n",addr);
    J_count++;
    break;

    default:
    printf("rs[%X], rt[%X], imm[%X]\n",rs,rt,imm);
    I_count++;
    break;
    }

    return ret;
}

int Control_Signal(){
    int ret = 1;

    // MUX
    if(opcode==0){
        RegDst = 1;
    } else{
        RegDst = 0;
    }

    if((opcode==J)||(opcode==JAL)){
        Jump = 1;
    } else{
        Jump = 0;
    }

    if((opcode==BEQ)||(opcode==BNE)){
        Branch = 1;
    } else{
        Branch = 0;
    }

    if(opcode==LW){
        MemtoReg = 1;
        MemRead = 1;
    } else{
        MemtoReg = 0;
        MemRead = 0;
    }

    if(opcode==SW){
        MemWrite = 1;
    } else{
        MemWrite = 0;
    }

    if((opcode!=0)&&(opcode!=BEQ)&&(opcode!=BNE)){
        ALUSrc = 1;
    } else{
        ALUSrc = 0;
    }


    if((opcode!=SW)&&(opcode!=BEQ)&&(opcode!=BNE)&&(opcode!=J)&&(opcode!=JR)){
        RegWrite = 1;
    } else{
        RegWrite = 0;
    }

    printf("RegDst :%d\tJump: %d\tBranch: %d\tMemRead: %d\tMemtoReg: %d\tMemWrite: %d\tALUSrc: %d\tRegWrite: %d\n",RegDst, Jump, Branch, MemRead, MemtoReg, MemWrite, ALUSrc, RegWrite);
    return ret;
}

int Sign_Extend(int imm){
    int SignExtImm;
    int SignBit = imm >> 15;
    if(SignBit == 1){
        SignExtImm = 0xFFFF0000 | imm;
    } else{
        SignExtImm = 0x0000FFFF & imm;
    }
    return SignExtImm;
}

int jump_Addr(){
    int j = ((PC+4)&0xF0000000)|(addr<<2);
    return j;
}

int branch_Addr(int imm){
    int b = Sign_Extend(imm) << 2;
    return b;
}

int R_Type(int func){
    switch(func){
    case ADD:
    R[rd] = (int)((int)R[rs] + (int)R[rt]);
    printf("ADD\nR%d(0x%X) = R%d + R%d\n", rd, R[rd], rs, rt);
    break;
        
    case ADDU:
    R[rd] = R[rs] + R[rt];
    printf("ADDU\nR%d(0x%X) = R%d + R%d\n", rd, R[rd], rs, rt);
    break;
        
    case AND:
    R[rd] = R[rs] & R[rt];
    printf("AND\nR%d(0x%X) = R%d & R%d\n", rd, R[rd], rs, rt);
    break;
        
    case JR:
    PC = R[rs];
    change_pc_val = 1;
    printf("JR\nPC = R%d(0x%X)\n", rs, R[rs]);
    break;

    case JALR:
    R[rd] = PC+4;
    PC = R[rs];
    change_pc_val = 1;
    printf("JALR\nJAL + PC = R%d(0x%X)\n", rs, R[rs]);
    break;

    case NOR:
    R[rd] = ~(R[rs]|R[rt]);
    printf("NOR\nR%d(0x%X) = R%d & R%d\n", rd, R[rd], rs, rt);
    break;

    case OR:
    R[rd] = R[rs]|R[rt];
    printf("OR\nR%d(0x%X) = R%d & R%d\n", rd, R[rd], rs, rt);
    break;

    case SLT:
    R[rd] = ((int)R[rs] < (int)R[rt]) ? 1:0;
    printf("SLT\nR%d = R%d(0x%X) < R%d(0x%X)\n", rd, rs, R[rs], rt, R[rt]);
    break;
        
    case SLTU:
    R[rd] = (R[rs] < R[rt]) ? 1:0;
    printf("SLTU\nR%d = R%d(0x%X) < R%d(0x%X)\n", rd, rs, R[rs], rt, R[rt]);
    break;
        
    case SLL:
    R[rd] = R[rt] << shamt;
    printf("SLL\nR%d(0x%X) = R%d(0x%X) << %d\n", rd, R[rd], rt, R[rt], shamt);
    break;
            
    case SRL:
    R[rd] = R[rt] >> shamt;            
    printf("SRL\nR%d(0x%X) = R%d(0x%X) >> %d\n", rd, R[rd], rt, R[rt], shamt);
    break;
        
    case SUB:
    R[rd] = (int)((int)R[rs] - (int)R[rt]);
    printf("SUB\nR%d(0x%X) = R%d - R%d\n", rd, R[rd], rs, rt);
    break;

    case SUBU:
    R[rd] = R[rs] - R[rt];
    printf("SUBU\nR%d(0x%X) = R%d - R%d\n", rd, R[rd], rs, rt);
    break;

    default:
    return -1;
    }

    return change_pc_val;
}

int J_Type(int opcode){
    unsigned int JumpAddr = jump_Addr();
    switch(opcode){
    case J:
    PC = JumpAddr;
    change_pc_val = 1;
    printf("J\nJumpAddr 0x%X\n", JumpAddr);
    break;

    case JAL:
    R[ra] = (unsigned int)PC+8;
    PC = JumpAddr;
    change_pc_val = 1;
    printf("JAL\nJumpAddr 0x%X, ra 0x%X\n", JumpAddr, R[ra]);
    break;

    default:
    return -1;
    }

    return change_pc_val;
}

int I_Type(int opcode){
    int ZeroExtImm = imm;
    int SignExtImm = Sign_Extend(imm);
    int BranchAddr = branch_Addr(imm);
    switch (opcode){
    case ADDI:
    R[rt] = (int)((int)R[rs]+SignExtImm);
    printf("ADDI\nR%d(0x%X) = R%d + (%d)\n", rd, R[rd], rs, SignExtImm);
    break;

    case ADDIU:
    R[rt] = (unsigned int)R[rs]+SignExtImm;
    printf("ADDIU\nR%d(0x%X) = R%d + (%d)\n", rd, R[rd], rs, SignExtImm);
    break;

    case ANDI:
    R[rt] = R[rs] & ZeroExtImm;
    printf("ANDI\nR%d(0x%X) = R%d & (%d)\n", rd, R[rd], rs, ZeroExtImm);
    break;

    case BEQ:
    if (R[rs] == R[rt]) {
        PC = PC+4+BranchAddr;
        change_pc_val = 1;
    }
    branch_count++;
    printf("BEQ\nif R%d(0x%X) == R%d(0x%X) -> BranchAddr 0x%X\n", rs, R[rs], rt, R[rt], BranchAddr);
    break;

    case BNE:
    if (R[rs] != R[rt]) {
        PC = PC+4+BranchAddr;
        change_pc_val = 1;
    }
    branch_count++;
    printf("BNE\nif R%d(0x%X) != R%d(0x%X) -> BranchAddr 0x%X\n", rs, R[rs], rt, R[rt], BranchAddr);
    break;

    case LBU:
    temp = R[rs]+(unsigned int)SignExtImm & 0x000000FF;
    R[rt] = (Memory[temp/u_Size]) & 0x000000FF;
    printf("LBU\nR%d(0x%X), Memory[0x%X]\n",rt,R[rt],SignExtImm);
    break;

    case LHU:
    temp = R[rs]+(unsigned int)SignExtImm & 0x0000FFFF;
    R[rt] = (Memory[temp/u_Size]) & 0x0000FFFF;
    printf("LHU\nR%d(0x%X), Memory[0x%X]\n",rt,R[rt],SignExtImm);
    break;  

    case LL:
    R[rt] = Memory[(R[rs]+(unsigned int)SignExtImm)/u_Size];
    printf("LL\nR%d(0x%X), Memory[0x%X]\n",rt,R[rt],SignExtImm);
    break;

    case LUI:
    R[rt] = imm<<16;
    printf("LUI\nR%d(0x%X)\n)", rt, R[rt]);
    break;

    case LW:
    R[rt] = Memory[(R[rs]+(unsigned int)SignExtImm)/u_Size];
    memAcc_count++;
    printf("LW\nR%d(0x%X), Memory[R%d(0x%X) + 0x%X]\n",rt,R[rt],rs,R[rs],SignExtImm);
    break;

    case ORI:
    R[rt] = R[rs] | ZeroExtImm;
    printf("ORI\nR%d(0x%X)= R%d | R%d\n", rt, R[rt], rs, ZeroExtImm);
    break;

    case SLTI:
    R[rt] = (R[rs] < SignExtImm) ? 1:0;
    printf("SLTI\nR%d(%d) < %d\n",rs,R[rs],SignExtImm);
    break;

    case SLTIU:
    R[rt] =(R[rs]< (unsigned int)SignExtImm) ? 1:0;
    printf ("SLTIU\nR%d(%d) < %d\n",rs, R[rs], SignExtImm);
    break; 

    case SB:
    temp= R[rs]+ (unsigned int)SignExtImm;
    temp2= Memory[temp/u_Size];
    temp2= temp2 & 0xFFFFFF00;
    temp2= (R[rt] & 0x000000FF)|temp2;
    Memory[temp/u_Size] = temp2;
    printf("SB\nR%d(0x%X), Memory[0x%X]\n", rt, R[rt], SignExtImm);
    break;

    case SC:
    temp = (unsigned int)SignExtImm;
    temp2 = R[rs]+temp;
    temp = R[rt];
    Memory[temp2/u_Size] = temp;
    R[rt] = 1; // R[rt] = (atomic) ? 1:0
    printf("SC\nR%d(0x%X), Memory[0x%X]\n", rt, R[rt], SignExtImm);
    break; 

    case SH:
    temp = R[rs]+(unsigned int)SignExtImm;
    temp2 = Memory[temp/u_Size];
    temp2 = temp2 & 0xFFFF0000;
    temp2 = (R[rt] & 0x0000FFFF) | temp2;
    Memory[temp/u_Size] = temp2;
    printf("SH\nR%d(0x%X), Memory[0x%X]\n", rt, R[rt], SignExtImm);
    break;  

    case SW:
    temp= R[rs]+(unsigned int)SignExtImm;
    Memory[temp/u_Size]= R[rt];
    memAcc_count++;
    printf("SW\nR%d(0x%X), Memory[R%d(0x%X)+0x%X]\n",rt , R[rt], rs, R[rs], SignExtImm);
    break;

    default:
    return -1;
    }
    return change_pc_val;
}

int Exe_and_WB(){
    printf("----Execute and Write Back----\n");
    int ret = 1;
    change_pc_val = 0;

    if(opcode == 0x00){
        change_pc_val = R_Type(func);
    } else if(opcode == 0x02 || opcode == 0x03){
        change_pc_val = J_Type(opcode);
    } else{
        change_pc_val = I_Type(opcode);
    }
    
    // PC Update
    if (change_pc_val == 0){
        PC= PC+4;
    } else if (change_pc_val == -1){
        ret = change_pc_val;
    }

    return ret;
}