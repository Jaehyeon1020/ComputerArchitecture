#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <arpa/inet.h> // htonl(엔디안 변환 little to big)
#include <stdlib.h>
#include <string.h>

int is_unknown_inst = 0;

typedef struct
{
    int reg[32]; //0x00000000 초기화
    int PC; // 0x00000000 초기화
}Register;

typedef struct
{
    int adrr; // 0x00000000 ~ 0x00010000
    char data[15]; //0xffffffff 초기화
}Instmemory;

typedef struct
{
    int addr; // 0x10000000 ~ 0x10010000
    int data; // 0xffffffff 초기화
}Datamemory;

typedef struct
{
	int opcode[6];
	int rsdecimal;
	int rtdecimal;
	int rddecimal;
	int funct[6];
	int imm[16];
	int addr[26];
	int IsEmpty;
	int IsNop;
}if_id;
if_id IF_ID;

void Clean_IF_ID()
{
	IF_ID.rsdecimal = 0;
	IF_ID.rtdecimal = 0;
	IF_ID.IsNop = 1;
	for(int i=0; i<6; i++)
	{
		IF_ID.opcode[i] = 0;
		IF_ID.funct[i] = 0;
	}
	for(int i=0; i<16; i++)
		IF_ID.imm[i] = 0;
	for(int i=0; i<26; i++)
		IF_ID.addr[i] = 0;
}

typedef struct
{
	int rsdecimal;
	int rtdecimal;
	int rddecimal;
	int opcode[6];
	int funct[6];
	int rsdata;
	int rtdata;
	int rddata;
	int immdata;
	unsigned int unsigned_immdata;
	char instname[6];
	int IsEmpty;
} id_ex;
id_ex ID_EX;

void Clean_ID_EX()
{
	ID_EX.rsdecimal = 0;
	ID_EX.rtdecimal = 0;
	ID_EX.rddecimal = 0;
	ID_EX.rsdata = 0;
	ID_EX.rtdata = 0;
	ID_EX.rddata = 0;
	ID_EX.immdata = 0;
	ID_EX.unsigned_immdata = 0;
	strcpy(ID_EX.instname,"nop");
	for(int i=0;i<6;i++)
	{
		ID_EX.opcode[i] = 0;
		ID_EX.funct[i] = 0;
	}
}

typedef struct
{
	int rsdecimal;
	int rtdecimal;
	int rddecimal;
	int opcode[6];
	int rsdata;
	int rtdata;
	int rddata;
	int immdata;
	unsigned int unsigned_immdata;
	char instname[6];
	int calculated_rs;
	int calculated_rt;
	int calculated_rd;
	int IsEmpty;
} ex_mem;
ex_mem EX_MEM;

typedef struct
{
	char instname[6];
	int rddecimal;
	int rtdecimal;
	int calculated_rs;
	int calculated_rt;
	int calculated_rd;
	int IsEmpty;
	int lwdata;
} mem_wb;
mem_wb MEM_WB;

unsigned int CheckSum = 0x00000000;

void regInit(Register * regi)
{
    for(int i=0; i<32; i++)
        regi->reg[i] = 0x00000000;
    regi->PC = 0x00000000;
}

int sqr2(int count) //2의 카운트 제곱
{
    if(count == 0)
        return 1;
    if(count == 1)
        return 2;

    int result = 2;

    for(int i=0; i<count-1; i++)
        result = result*2;
    return result;
}

int binTodecimal(int * binary,int len) // 부호 있는 숫자 계산
{
    int isnegative = 0;
    if(binary[0]==1) isnegative = 1;

    if(isnegative == 1)//MSB=1
    {
        for(int i=0; i<len; i++)
        {
            if(binary[i]==0) binary[i]=1;
            else binary[i]=0;
        }
        binary[len-1] += 1;

        for(int i=len-1; i>=0; i--)
        {
            if(binary[i]==2)
            {
                binary[i]=0;
                binary[i-1] += 1;
            }
            else break;
        }

        int decimal = 0;
        for(int i=0; i<len; i++)
            decimal = decimal + binary[i]*sqr2(len-1-i);

        return (-1)*decimal;
    }
    else if(isnegative == 0)
    {
        int decimal = 0;
        for(int i=0; i<len; i++)
            decimal = decimal + binary[i]*sqr2(len-1-i);
        return decimal;
    }
}

void PipelineRegInit(if_id * ifreg, id_ex * idreg, ex_mem * exreg, mem_wb * memreg)
{
	for(int i=0; i<6; i++)
	{
		ifreg->opcode[i] = 0;
		ifreg->funct[i] = 0;
		idreg->opcode[i] = 0;
		idreg->funct[i] = 0;
		exreg->opcode[i] = 0;
	}

	for(int i=0; i<16; i++)
		ifreg->imm[i] = 0;

	for(int i=0; i<26; i++)
		ifreg->addr[i] = 0;

	ifreg->rsdecimal = 0;
	ifreg->rtdecimal = 0;
	ifreg->rddecimal = 0;
	ifreg->IsNop = 1;

	idreg->rsdecimal = 0;
	idreg->rtdecimal = 0;
	idreg->rddecimal = 0;
	idreg->rsdata = 0;
	idreg->rtdata = 0;
	idreg->rddata = 0;
	idreg->immdata = 0;
	idreg->unsigned_immdata = 0;

	exreg->rsdecimal = 0;
	exreg->rtdecimal = 0;
	exreg->rddecimal = 0;
	exreg->rsdata = 0;
	exreg->rtdata = 0;
	exreg->rddata = 0;
	exreg->immdata = 0;
	exreg->unsigned_immdata = 0;
	exreg->calculated_rs = 0;
	exreg->calculated_rt = 0;
	exreg->calculated_rd = 0;

	memreg->rddecimal = 0;
	memreg->rtdecimal = 0;
	memreg->calculated_rs = 0;
	memreg->calculated_rt = 0;
	memreg->calculated_rd = 0;

	ifreg->IsEmpty = 1;
	idreg->IsEmpty = 1;
	exreg->IsEmpty = 1;
	memreg->IsEmpty = 1;
}

int is_load_use_hazard = 0;
int is_mispredicted = 0;
void IFstage(char * hexcode, Register * reg)
{
	if(is_load_use_hazard == 1)
	{
		is_load_use_hazard = 0;
		return;
	}

	if(is_mispredicted == 1)
	{
		is_mispredicted = 0;
		return;
	}

	//printf("----------IF stage in----------\n");
	IF_ID.IsEmpty = 0;
	int bincode[32] = {0,};
    int current = 0;
    
    for(int i=0; i<8; i++)
    {

        if(hexcode[i] == '0')
        {
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == '1')
        {
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 1;
        }
        else if(hexcode[i] == '2')
        {
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == '3')
        {
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 1;
        }
        else if(hexcode[i] == '4')
        {
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == '5')
        {
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 1;
		}
		else if(hexcode[i] == '6')
        {
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == '7')
        {
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 1;
        }
        else if(hexcode[i] == '8')
        {
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == '9')
        {
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 0;
            bincode[current++] = 1;
        }
        else if(hexcode[i] == 'a' || hexcode[i] == 'A')
        {
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == 'b' || hexcode[i] == 'B')
        {
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 1;
            bincode[current++] = 1;
        }
		else if(hexcode[i] == 'c' || hexcode[i] == 'C')
        {
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == 'd' || hexcode[i] == 'D')
        {
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 0;
            bincode[current++] = 1;
        }
        else if(hexcode[i] == 'e' || hexcode[i] == 'E')
        {
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 0;
        }
        else if(hexcode[i] == 'f' || hexcode[i] == 'F')
        {
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 1;
            bincode[current++] = 1;
        }
	}// hexcode(instruction)을 binary code로 저장

	for(int i=0; i<32; i++)
	{
		if(bincode[i] != 0)
			IF_ID.IsNop = 0;
	}

    int opcode[6] = {bincode[0],bincode[1],bincode[2],bincode[3],bincode[4],bincode[5]};
    int rs[5] = {bincode[6],bincode[7],bincode[8],bincode[9],bincode[10]};
    int rt[5] = {bincode[11],bincode[12],bincode[13],bincode[14],bincode[15]};
	int rd[5] = {bincode[16],bincode[17],bincode[18],bincode[19],bincode[20]};
    int funct[6] = {bincode[26],bincode[27],bincode[28],bincode[29],bincode[30],bincode[31]};
	int imm[16] = {bincode[16],bincode[17],bincode[18],bincode[19],bincode[20],bincode[21],bincode[22],bincode[23],bincode[24],bincode[25],bincode[26],bincode[27],bincode[28],bincode[29],bincode[30],bincode[31]};
	int address[26];
    for(int i=0; i<26; i++)
        address[i] = bincode[i+6];

	int addressdata = binTodecimal(address,26);
	
	memcpy(IF_ID.opcode,opcode,sizeof(int)*6);
	memcpy(IF_ID.funct,funct,sizeof(int)*6);
	memcpy(IF_ID.imm,imm,sizeof(int)*16);
	memcpy(IF_ID.addr,address,sizeof(int)*26);

	IF_ID.rsdecimal = 0; IF_ID.rtdecimal = 0; IF_ID.rddecimal = 0;
    for(int i=0; i<5; i++)
    {
		IF_ID.rsdecimal = IF_ID.rsdecimal + rs[i]*sqr2(4-i);
        IF_ID.rtdecimal = IF_ID.rtdecimal + rt[i]*sqr2(4-i);
        IF_ID.rddecimal = IF_ID.rddecimal + rd[i]*sqr2(4-i);
    }

	if(opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==0 && opcode[4]==1 && opcode[5]==0)
		reg->PC = ((reg->PC>>28)<<28) + (addressdata<<2); // IF Jump inst
	else reg->PC = reg->PC + 4;

	//printf("----------IF stage out----------\n");
}

int branch_rsdata = 0;
int branch_rtdata = 0;
int branch_rs_forwarded = 0;
int branch_rt_forwarded = 0;
void IDstage(Register * reg)
{
	if(IF_ID.IsEmpty == 1)
		return;
	if(is_load_use_hazard == 1)
	{
		CheckSum = ((CheckSum<<1) | (CheckSum>>31))^(reg->reg[IF_ID.rsdecimal]);
		return;
	}

	ID_EX.IsEmpty = 0;

	//printf("----------IDstage in----------\n");
	ID_EX.rsdecimal = IF_ID.rsdecimal;
	ID_EX.rtdecimal = IF_ID.rtdecimal;
	ID_EX.rddecimal = IF_ID.rddecimal;
	ID_EX.rsdata = reg->reg[ID_EX.rsdecimal];
	ID_EX.rtdata = reg->reg[ID_EX.rtdecimal];
	ID_EX.rddata = reg->reg[ID_EX.rddecimal];
	ID_EX.immdata = binTodecimal(IF_ID.imm,16);
	ID_EX.unsigned_immdata = ID_EX.immdata;
	ID_EX.unsigned_immdata = (ID_EX.unsigned_immdata<<16)>>16;

	CheckSum = ((CheckSum<<1) | (CheckSum>>31))^(reg->reg[IF_ID.rsdecimal]);

	int isRtype=0; int isItype=0; int isJtype=0;
	int opcode[6];
	int funct[6];
	memcpy(opcode,IF_ID.opcode,sizeof(int)*6);
	memcpy(funct,IF_ID.funct,sizeof(int)*6);

	if(opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==0 && opcode[4]==0 && opcode[5]==0)
            isRtype = 1;
    else if((opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==0 && opcode[4]==1 && opcode[5]==0) || (opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==0 && opcode[4]==1 && opcode[5]==1))
            isJtype = 1;
    else
            isItype = 1;

	if(isRtype == 1)
    {
        //add
        if(funct[0]==1 && funct[1]==0 && funct[2]==0 && funct[3]==0 && funct[4]==0 && funct[5]==0)
        {
            strcpy(ID_EX.instname,"add");
            //printf("add $%d, $%d, $%d\n",rddecimal,rsdecimal,rtdecimal");
        }

        //and
        else if(funct[0]==1 && funct[1]==0 && funct[2]==0 && funct[3]==1 && funct[4]==0 && funct[5]==0)
        {
            strcpy(ID_EX.instname,"and");
            //printf("and $%d, $%d, $%d\n",rddecimal,rsdecimal,rtdecimal);
        }
		//or
        else if(funct[0]==1 && funct[1]==0 && funct[2]==0 && funct[3]==1 && funct[4]==0 && funct[5]==1)
        {
            strcpy(ID_EX.instname,"or");
            //printf("or $%d, $%d, $%d\n",rddecimal,rsdecimal,rtdecimal);
        }

        //slt
        else if(funct[0]==1 && funct[1]==0 && funct[2]==1 && funct[3]==0 && funct[4]==1 && funct[5]==0)
        {
            strcpy(ID_EX.instname,"slt");
            //printf("slt $%d, $%d, $%d\n",rddecimal,rsdecimal,rtdecimal);
        }

        //sub
        else if(funct[0]==1 && funct[1]==0 && funct[2]==0 && funct[3]==0 && funct[4]==1 && funct[5]==0)
        {
            strcpy(ID_EX.instname,"sub");
            //printf("sub $%d, $%d, $%d\n",rddecimal, rsdecimal, rtdecimal);
        }

		//nop
		else if(IF_ID.IsNop == 1)
		{
			strcpy(ID_EX.instname,"nop");
		}

        else
		{
            strcpy(ID_EX.instname,"unknown");
            //printf("is_unknown_inst = 1\n");
		}
    }//R-type

    if(isItype == 1)
    {
        //addi(signed int)
        if(opcode[0]==0 && opcode[1]==0 && opcode[2]==1 && opcode[3]==0 && opcode[4]==0 && opcode[5]==0)
        {
            strcpy(ID_EX.instname,"addi");
            //printf("addi $%d, $%d, %d\n",IF_ID.rtdecimal,IF_ID.rsdecimal,ID_EX.immdata);
        }

        //andi
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==1 && opcode[3]==1 && opcode[4]==0 && opcode[5]==0)
        {
            strcpy(ID_EX.instname,"andi");
            //printf("andi $%d, $%d, 0x%x\n",rtdecimal,rsdecimal,un_immdecimal);
        }

        //beq
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==1 && opcode[4]==0 && opcode[5]==0)
        {
            strcpy(ID_EX.instname,"beq");
            //printf("beq $%d, $%d, %d\n",rsdecimal,rtdecimal,immdecimal);
        }

        //bne
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==0 && opcode[3]==1 && opcode[4]==0 && opcode[5]==1)
        {
            strcpy(ID_EX.instname,"bne");
            //printf("bne $%d, $%d, %d\n",rsdecimal,rtdecimal,immdecimal);
        }
        //lui
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==1 && opcode[3]==1 && opcode[4]==1 && opcode[5]==1)
        {
            strcpy(ID_EX.instname,"lui");
            //printf("lui $%d, %d\n",rtdecimal,immdecimal);
        }

        //lw
        else if(opcode[0]==1 && opcode[1]==0 && opcode[2]==0 && opcode[3]==0 && opcode[4]==1 && opcode[5]==1)
        {
            strcpy(ID_EX.instname,"lw");
		}

        //ori
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==1 && opcode[3]==1 && opcode[4]==0 && opcode[5]==1)
        {
            strcpy(ID_EX.instname,"ori");
            //printf("ori $%d, $%d, %d\n",rtdecimal,rsdecimal,un_immdecimal);
        }

        //slti
        else if(opcode[0]==0 && opcode[1]==0 && opcode[2]==1 && opcode[3]==0 && opcode[4]==1 && opcode[5]==0)
        {
            strcpy(ID_EX.instname,"slti");
            //printf("slti $%d, $%d, %d\n",rtdecimal,rsdecimal,immdecimal);
        }

        //sw
        else if(opcode[0]==1 && opcode[1]==0 && opcode[2]==1 && opcode[3]==0 && opcode[4]==1 && opcode[5]==1)
        {
            strcpy(ID_EX.instname,"sw");
		}
        
        else
		{
            strcpy(ID_EX.instname,"unknown");
            //printf("is_unknown_inst = 1\n");
		}
    }//I-type

    if(isJtype == 1)
    {   
        // J(Jump)
        if(opcode[5] == 0)
        {
            strcpy(ID_EX.instname,"j"); // jump inst 구현 안되어있음
            //printf("j %d\n",addressdecimal);
        }
        else
		{
            strcpy(ID_EX.instname,"unknown");
            //printf("is_unknown_inst = 1\n");
		}
    }//J-type

	//printf("This instruction in ID stage : %s\n",ID_EX.instname);

	if(strcmp(ID_EX.instname,"beq")==0)
	{
		if(branch_rs_forwarded == 1 && branch_rt_forwarded == 1)
		{
			branch_rs_forwarded = 0;
			branch_rt_forwarded = 0;
			
			if(branch_rsdata == branch_rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = reg->PC + ID_EX.immdata*4;
				branch_rsdata = 0;
				branch_rtdata = 0;
			}
		}
		else if(branch_rs_forwarded == 1 && branch_rt_forwarded == 0)
		{
			branch_rs_forwarded = 0;

			if(branch_rsdata == ID_EX.rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = reg->PC + ID_EX.immdata*4;
				branch_rsdata = 0;
			}
		}
		else if(branch_rs_forwarded == 0 && branch_rt_forwarded == 1)
		{
			branch_rt_forwarded = 0;

			if(ID_EX.rsdata == branch_rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = reg->PC + ID_EX.immdata*4;
				branch_rtdata = 0;
			}
		}
		else
		{
			if(ID_EX.rsdata == ID_EX.rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = reg->PC + ID_EX.immdata*4;
			}
		}
	}
	else if(strcmp(ID_EX.instname,"bne")==0)
	{
		if(branch_rs_forwarded == 1 && branch_rt_forwarded == 1)
		{
			branch_rs_forwarded = 0;
			branch_rt_forwarded = 0;
			
			if(branch_rsdata != branch_rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = reg->PC + ID_EX.immdata*4;
			}
		}
		else if(branch_rs_forwarded == 1 && branch_rt_forwarded == 0)
		{
			branch_rs_forwarded = 0;

			if(branch_rsdata != ID_EX.rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = (reg->PC) + (ID_EX.immdata<<2);
			}
		}
		else if(branch_rs_forwarded == 0 && branch_rt_forwarded == 1)
		{
			branch_rt_forwarded = 0;

			if(ID_EX.rsdata != branch_rtdata)
			{
				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = (reg->PC) + (ID_EX.immdata<<2);
			}
		}
		else
		{
			if(ID_EX.rsdata != ID_EX.rtdata)
			{
				//printf("BRANCH!!!!\n");

				Clean_IF_ID();
				is_mispredicted = 1;
				reg->PC = (reg->PC) + (ID_EX.immdata<<2);
			}
		}
	} // beq, bne handling

	//printf("----------IDstage out----------\n");
}

void EXstage()
{
	if(ID_EX.IsEmpty == 1)
		return;

	EX_MEM.IsEmpty = 0;

	//printf("----------EXstage in----------\n");


	int calculated_rd=0;
	int calculated_rt=0;
	int calculated_rs=0;

	if(strcmp(ID_EX.instname,"add")==0)
	{
		calculated_rd = ID_EX.rsdata + ID_EX.rtdata;
	}
	else if(strcmp(ID_EX.instname,"sub")==0)
	{
		calculated_rd = ID_EX.rsdata - ID_EX.rtdata;
	}
	else if(strcmp(ID_EX.instname,"and")==0)
	{
		calculated_rd = ID_EX.rsdata & ID_EX.rtdata;
	}
	else if(strcmp(ID_EX.instname,"or")==0)
	{
		calculated_rd = ID_EX.rsdata | ID_EX.rtdata;
	}
	else if(strcmp(ID_EX.instname,"slt")==0)
	{
		if(ID_EX.rsdata < ID_EX.rtdata)
			calculated_rd = 1;
		else calculated_rd = 0;
	}
	else if(strcmp(ID_EX.instname,"addi")==0)
	{
		calculated_rt = ID_EX.rsdata + ID_EX.immdata;
		//printf("cal : rs $%d(%d) + imm(%d)\n",ID_EX.rsdecimal,ID_EX.rsdata,ID_EX.immdata);
	}
	else if(strcmp(ID_EX.instname,"andi")==0)
	{
		calculated_rt = ID_EX.rsdata & ID_EX.unsigned_immdata;
	}
	else if(strcmp(ID_EX.instname,"ori")==0)
	{
		calculated_rt = ID_EX.rsdata | ID_EX.unsigned_immdata;
	}
	else if(strcmp(ID_EX.instname,"slti")==0)
	{
		if(ID_EX.rsdata < ID_EX.immdata)
			calculated_rt = 1;
		else calculated_rt = 0;
	}
	else if(strcmp(ID_EX.instname,"lui")==0)
	{
		calculated_rt = ID_EX.immdata<<16;
	}

	EX_MEM.calculated_rs = calculated_rs;
	EX_MEM.calculated_rt = calculated_rt;
	EX_MEM.calculated_rd = calculated_rd;

	EX_MEM.rsdecimal = ID_EX.rsdecimal;
	EX_MEM.rtdecimal = ID_EX.rtdecimal;
	EX_MEM.rddecimal = ID_EX.rddecimal;
	EX_MEM.rsdata = ID_EX.rsdata;
	EX_MEM.rtdata = ID_EX.rtdata;
	EX_MEM.rddata = ID_EX.rddata;
	EX_MEM.immdata = ID_EX.immdata;
	EX_MEM.unsigned_immdata = ID_EX.unsigned_immdata;
	memcpy(EX_MEM.opcode,ID_EX.opcode,sizeof(int)*6);
	strcpy(EX_MEM.instname,ID_EX.instname);

	if(strcmp(ID_EX.instname,"lw")==0)
	{
		if(ID_EX.rtdecimal == IF_ID.rsdecimal)
		{
			Clean_ID_EX();
			is_load_use_hazard = 1;
		}
		else if(ID_EX.rtdecimal == IF_ID.rtdecimal)
		{
			Clean_ID_EX();
			is_load_use_hazard = 1;
		}
	}

	// branch inst의 datahazard detection
	if((IF_ID.opcode[0]==0 && IF_ID.opcode[1]==0 && IF_ID.opcode[2]==0 && IF_ID.opcode[3] == 1 && IF_ID.opcode[4]==0 && IF_ID.opcode[5]==0) || (IF_ID.opcode[0]==0 && IF_ID.opcode[1]==0 && IF_ID.opcode[2]==0 && IF_ID.opcode[3]==1 && IF_ID.opcode[4]==0 && IF_ID.opcode[5]==1))
	{
		if((strcmp(ID_EX.instname,"add")==0)|| (strcmp(ID_EX.instname,"sub")==0) || (strcmp(ID_EX.instname,"and")==0) || (strcmp(ID_EX.instname,"or")==0) || (strcmp(ID_EX.instname,"slt")==0)) // beq 전에 Rtype
		{
			if(ID_EX.rddecimal == IF_ID.rtdecimal)
			{
				branch_rtdata = calculated_rd;
				branch_rt_forwarded = 1;
			}
			else if(ID_EX.rddecimal == IF_ID.rsdecimal)
			{
				branch_rsdata = calculated_rd;
				branch_rs_forwarded = 1;f
			}
		}
		else if((strcmp(ID_EX.instname,"addi")==0) || (strcmp(ID_EX.instname,"andi")==0) || (strcmp(ID_EX.instname,"ori")==0) || (strcmp(ID_EX.instname,"slti")==0) || (strcmp(ID_EX.instname,"lui")==0)) // beq 전에 Itype
		{
			if(ID_EX.rtdecimal == IF_ID.rtdecimal)
			{
				branch_rtdata = calculated_rt;
				branch_rt_forwarded = 1;
			}
			else if(ID_EX.rtdecimal == IF_ID.rsdecimal)
			{
				branch_rsdata = calculated_rt;
				branch_rt_forwarded = 1;
			}
		}
	}

	//printf("----------EX out----------\n");
} // lw, sw, beq, bne, j는 EXstage에서 처리 안함

void MEMstage(Register * reg,Datamemory * data)
{
	if(EX_MEM.IsEmpty == 1)
		return;

	MEM_WB.IsEmpty = 0;

	//printf("---------MEMstage in-----------\n");

	// Data Hazard Handling
	if((strcmp(ID_EX.instname,"add")==0)|| (strcmp(ID_EX.instname,"sub")==0) || (strcmp(ID_EX.instname,"and")==0) || (strcmp(ID_EX.instname,"or")==0) || (strcmp(ID_EX.instname,"slt")==0)) // Rtype
	{
		//printf("Enter Data Hazard R\n");
		
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // Rtype 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
			else if(EX_MEM.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rd;
			}
		}

		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // Rtype 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rd;
			}
			else if(MEM_WB.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rd;
			}
		}

		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // Rtype 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
			else if(EX_MEM.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // Rtype 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
			else if(MEM_WB.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rt;
			}
		}
	}
	else if((strcmp(ID_EX.instname,"addi")==0) || (strcmp(ID_EX.instname,"andi")==0) || (strcmp(ID_EX.instname,"ori")==0) || (strcmp(ID_EX.instname,"slti")==0) || (strcmp(ID_EX.instname,"lui")==0)) // Itype
	{
		//printf("Enter Data Hazard I\n");
		
		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // Itype 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // Itype 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
		}

		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // Itype 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
		}
		
		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // Itype 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rd;
			}
		}
	}
	else if(strcmp(ID_EX.instname,"sw")==0)
	{
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // sw 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rd;
			}
			else if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata == EX_MEM.calculated_rd;
			}
		}

		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // sw 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rd;
			}
			else if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata == MEM_WB.calculated_rd;
			}
		}

		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // sw 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rt;
			}
			else if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // sw 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rt;
			}
			else if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
		}
	}
	else if(strcmp(ID_EX.instname,"lw")==0)
	{
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // lw 전에 R type
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
		}
		else if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // lw 전에 I type
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				//printf("MEMstage lw - Itype detection : ID_EX.rsdata %d, EX_MEM.calculated_rt %d\n",ID_EX.rsdata,EX_MEM.calculated_rt);
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}
	}// Data Hazard Handling


	strcpy(MEM_WB.instname,EX_MEM.instname);

	int lwdata;
	if(strcmp(EX_MEM.instname,"lw")==0)
	{
		lwdata = data[EX_MEM.rsdata + EX_MEM.immdata - 0x10000000].data;
	}
	else if(strcmp(EX_MEM.instname,"sw")==0)
	{
		data[EX_MEM.rsdata + EX_MEM.immdata - 0x10000000].data = EX_MEM.rtdata;
	}

	MEM_WB.rddecimal = EX_MEM.rddecimal;
	MEM_WB.rtdecimal = EX_MEM.rtdecimal;
	MEM_WB.calculated_rs = EX_MEM.calculated_rs;
	MEM_WB.calculated_rt = EX_MEM.calculated_rt;
	MEM_WB.calculated_rd = EX_MEM.calculated_rd;
	MEM_WB.lwdata = lwdata;

	// branch inst의 datahazard detection
	if((IF_ID.opcode[0]==0 && IF_ID.opcode[1]==0 && IF_ID.opcode[2]==0 && IF_ID.opcode[3] == 1 && IF_ID.opcode[4]==0 && IF_ID.opcode[5]==0) || (IF_ID.opcode[0]==0 && IF_ID.opcode[1]==0 && IF_ID.opcode[2]==0 && IF_ID.opcode[3]==1 && IF_ID.opcode[4]==0 && IF_ID.opcode[5]==1))
	{
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // beq 전전에 Rtype
		{
			if(EX_MEM.rddecimal == IF_ID.rtdecimal)
			{
				branch_rtdata = EX_MEM.calculated_rd;
				branch_rt_forwarded = 1;
			}
			else if(EX_MEM.rddecimal == IF_ID.rsdecimal)
			{
				branch_rsdata = EX_MEM.calculated_rd;
				branch_rs_forwarded = 1;
			}
		}
		else if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // beq 전전에 Itype
		{
			if(EX_MEM.rtdecimal == IF_ID.rtdecimal)
			{
				branch_rtdata = EX_MEM.calculated_rt;
				branch_rt_forwarded = 1;
			}
			else if(EX_MEM.rtdecimal == IF_ID.rsdecimal)
			{
				branch_rsdata = EX_MEM.calculated_rt;
				branch_rs_forwarded = 1;
			}
		}
		else if(strcmp(EX_MEM.instname,"lw")==0)
		{
			if(EX_MEM.rtdecimal == IF_ID.rtdecimal)
			{
				branch_rtdata = lwdata;
				branch_rt_forwarded = 1;
			}
			else if(EX_MEM.rtdecimal == IF_ID.rsdecimal)
			{
				branch_rsdata = lwdata;
				branch_rs_forwarded = 1;
			}
		}
	}

	//printf("----------MEMstage out-----------\n");
}

void WBstage(Register * reg)
{
	if(MEM_WB.IsEmpty == 1)
		return;

	//printf("----------WBstage in-----------\n");
	//printf("MEM_WB.instname : %s\n",MEM_WB.instname);

	// Data Hazard Handling
	if((strcmp(ID_EX.instname,"add")==0)|| (strcmp(ID_EX.instname,"sub")==0) || (strcmp(ID_EX.instname,"and")==0) || (strcmp(ID_EX.instname,"or")==0) || (strcmp(ID_EX.instname,"slt")==0)) // Rtype
	{
		//printf("Enter Data Hazard R\n");
		
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // Rtype 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
			else if(EX_MEM.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rd;
			}
		}

		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // Rtype 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rd;
			}
			else if(MEM_WB.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rd;
			}
		}

		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // Rtype 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
			else if(EX_MEM.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // Rtype 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
			else if(MEM_WB.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rt;
			}
		}
	}
	else if((strcmp(ID_EX.instname,"addi")==0) || (strcmp(ID_EX.instname,"andi")==0) || (strcmp(ID_EX.instname,"ori")==0) || (strcmp(ID_EX.instname,"slti")==0) || (strcmp(ID_EX.instname,"lui")==0)) // Itype
	{
		//printf("Enter Data Hazard I\n");
		
		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // Itype 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // Itype 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
		}

		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // Itype 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
		}
		
		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // Itype 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rd;
			}
		}
	}
	else if(strcmp(ID_EX.instname,"sw")==0)
	{
		if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // sw 전에 Rtype
		{
			if(EX_MEM.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rd;
			}
			else if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata == EX_MEM.calculated_rd;
			}
		}

		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // sw 전전에 Rtype
		{
			if(MEM_WB.rddecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rd;
			}
			else if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata == MEM_WB.calculated_rd;
			}
		}

		if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // sw 전에 Itype
		{
			if(EX_MEM.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = EX_MEM.calculated_rt;
			}
			else if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}

		if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0)) // sw 전전에 Itype
		{
			if(MEM_WB.rtdecimal == ID_EX.rtdecimal)
			{
				ID_EX.rtdata = MEM_WB.calculated_rt;
			}
			else if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
		}
	}
	else if(strcmp(ID_EX.instname,"lw")==0)
	{
		/*if((strcmp(EX_MEM.instname,"add")==0)|| (strcmp(EX_MEM.instname,"sub")==0) || (strcmp(EX_MEM.instname,"and")==0) || (strcmp(EX_MEM.instname,"or")==0) || (strcmp(EX_MEM.instname,"slt")==0)) // lw 전에 R type
		{
			if(EX_MEM.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rd;
			}
		}
		else if((strcmp(EX_MEM.instname,"addi")==0) || (strcmp(EX_MEM.instname,"andi")==0) || (strcmp(EX_MEM.instname,"ori")==0) || (strcmp(EX_MEM.instname,"slti")==0) || (strcmp(EX_MEM.instname,"lui")==0)) // lw 전에 I type
		{
			if(EX_MEM.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = EX_MEM.calculated_rt;
			}
		}*/
		if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0)) // lw 전전에 R type
		{
			if(MEM_WB.rddecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rd;
			}
		}
		else if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui")==0))
		{
			if(MEM_WB.rtdecimal == ID_EX.rsdecimal)
			{
				ID_EX.rsdata = MEM_WB.calculated_rt;
			}
		}
	} // DATA hazard handling



	if((strcmp(MEM_WB.instname,"add")==0)|| (strcmp(MEM_WB.instname,"sub")==0) || (strcmp(MEM_WB.instname,"and")==0) || (strcmp(MEM_WB.instname,"or")==0) || (strcmp(MEM_WB.instname,"slt")==0))
	{
		//printf("WB_Rtype. saving %d\n",MEM_WB.calculated_rd);
		reg->reg[MEM_WB.rddecimal] = MEM_WB.calculated_rd;
	}

	else if((strcmp(MEM_WB.instname,"addi")==0) || (strcmp(MEM_WB.instname,"andi")==0) || (strcmp(MEM_WB.instname,"ori")==0) || (strcmp(MEM_WB.instname,"slti")==0) || (strcmp(MEM_WB.instname,"lui") == 0))
	{
		//printf("$%d = %d\n",MEM_WB.rtdecimal,MEM_WB.calculated_rt);
		reg->reg[MEM_WB.rtdecimal] = MEM_WB.calculated_rt;
	}
	else if((strcmp(MEM_WB.instname,"lw")==0))
	{
		reg->reg[MEM_WB.rtdecimal] = MEM_WB.lwdata;

		if(MEM_WB.rtdecimal == ID_EX.rsdecimal) // lw forwarding
			ID_EX.rsdata = MEM_WB.lwdata;
		else if(MEM_WB.rtdecimal == ID_EX.rtdecimal)
			ID_EX.rtdata = MEM_WB.lwdata;
	}

	//printf("----------WBstage out----------\n");
}

void ExecutePipeline(Register * reg, Instmemory * instmem, Datamemory * datamem, int cycle)
{
	for(int i=0; i<cycle; i++)
	{
		if(is_load_use_hazard == 1) is_load_use_hazard = 0;

		WBstage(reg);
		MEMstage(reg,datamem);
		EXstage();
		IDstage(reg);
		IFstage(instmem[reg->PC].data,reg);
		//printf("----------Cycle %d over----------\n",i+1);
	}
}

int main(int argc, char * argv[])
{
	PipelineRegInit(&IF_ID, &ID_EX, &EX_MEM, &MEM_WB);

    Register * registers;
    Instmemory * instmemories;
    Datamemory * datamemories;
    registers = (Register*)malloc(sizeof(Register));
    instmemories = (Instmemory*)malloc(sizeof(Instmemory)*0x10001);
    datamemories = (Datamemory*)malloc(sizeof(Datamemory)*0x10001);
	
    int instcnt = 0;
    
    regInit(registers);
    for(int i=0x0; i<0x10001; i++)
    {
       // instmemories[i].data = "0xffffffff";
		strcpy(instmemories[i].data,"0xffffffff");
        instmemories[i].adrr = 0x00000000 + i;
        
        datamemories[i].data = 0xffffffff;
        datamemories[i].addr = 0x10000000 + i;
    }
    
    int third_argument = atoi(argv[2]);
    int until_third_argument = 0;
    
    FILE * bin;
    int hexcode;
    int * hexcodeptr = &hexcode;
    char hexcodestr[9];
    
    bin = fopen(argv[1],"rb");
    if(bin == NULL) return 1;
  
	while(feof(bin) == 0)
	{
		fread(hexcodeptr, 1, sizeof(int), bin);
		if(feof(bin) == 1)
			break;

		*hexcodeptr = htonl(*hexcodeptr);
		sprintf(hexcodestr, "%08x", *hexcodeptr);

		instmemories[instcnt].adrr = instcnt;
		strcpy(instmemories[instcnt].data,hexcodestr);

		instcnt += 4;
	}

	ExecutePipeline(registers,instmemories,datamemories,third_argument);
    
    if(strcmp(argv[3],"reg") == 0)
    {
		printf("Checksum: 0x%08x\n",CheckSum);
        for(int i=0; i<32; i++)
            printf("$%d: 0x%08x\n",i,registers->reg[i]);
        printf("PC: 0x%08x\n",registers->PC-4);
    }
    else if(strcmp(argv[3],"mem") == 0)
    {
		int fourth_argument = strtol(argv[4], NULL, 16);
        int fifth_argument = atoi(argv[5]);
        
        for(int i=0; i<fifth_argument; i++)
        {
            printf("0x%08x\n",datamemories[fourth_argument-0x10000000 + i*4].data);
        }
    }

    return 0;
}
