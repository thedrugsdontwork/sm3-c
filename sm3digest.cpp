#include <iostream>
#include "sm3digest.h"
#include <assert.h>

word FF_j(word x,word y,word z,int j){
    return j<=15 && j>=0? x^y^z: (x&y)|(x&z)|(y&z);
}
word GG_j(word x,word y,word z,int j){
    return j<=15 && j>=0? x^y^z : (x&y)|((~x)&z);
}

word P_0(word x){
    return x^rot_left(x,9)^rot_left(x,17);
}
word P_1(word x){
    return x^rot_left(x,15)^rot_left(x,23);
}
//step1:fill step2:compress
FillMsg *fill(char *msg,long len){
    long long realLen=len*8,fillLen,expLen;
    char *p,*q;
    fillLen=448-(realLen%512+1);
    expLen=realLen+64+fillLen+1;
    p=(char*) malloc(sizeof(char)*(expLen/8));
    memcpy(p,msg,len);
    q=p+len;
    *q++=0x80;
    for(int i =1;i<=(fillLen/8);i++){
        *(q++)=0x00;
    }
    for(int i=1;i<=8;i++){
        *(q++)=0xff&((realLen>>(8-i)*8)&0xff);
    }
    FillMsg *f=(FillMsg*) malloc(sizeof(FillMsg));
    f->bitSize=expLen;
    f->msg=p;
    return f;
}
word *expand(word *block){
    word *array;
    array=(word*)malloc(sizeof(word)*132);
    memcpy(array,block, sizeof(word)*16);
    for(int j=16;j<68;j++){
        *(array+j)=  P_1(*(array+j-16)^*(array+j-9)^rot_left(*(array+j-3),15))^rot_left(*(array+j-13),7)^*(array+j-6);
    }
    for(int j=0;j<64;j++){
        *(array+68+j)=  *(array+j)^*(array+j+4);
    }

    return array;
}
word *compress(word* v,word* b){
    word ss1,ss2,tt1,tt2,A,B,C,D,E,F,G,H,*array,*res;
    A=*(v);
    B=*(v+1);
    C=*(v+2);
    D=*(v+3);
    E=*(v+4);
    F=*(v+5);
    G=*(v+6);
    H=*(v+7);
    array= expand(b);
    printf("expand:\n");
    for(int i=0;i<132;i++){
        if(i%8==0&&i>0) printf("\n");
        printf("%08x ",*(array+i));

    }
    printf("\ncompress:\n");
    printf("    %08c,%08c,%08c,%08c,%08c,%08c,%08c,%08c\n",'A','B','C','D','E','F','G','H');
    printf("%02d  %08x %08x %08x %08x %08x %08x %08x %08x \n",-1,A,B,C,D,E,F,G,H);
    for(int j=0;j<64;j++){
        ss1=rot_left((rot_left(A,12)+E+rot_left(T_j(j),j)),7);
        ss2=ss1^rot_left(A,12);
        tt1= FF_j(A,B,C,j)+D+ss2+*(array+68+j);
        tt2= GG_j(E,F,G,j)+H+ss1+*(array+j);
        D=C;
        C= rot_left(B,9);
        B=A;
        A=tt1;
        H=G;
        G= rot_left(F,19);
        F=E;
        E= P_0(tt2);
        printf("%02d  %08x %08x %08x %08x %08x %08x %08x %08x \n",j,A ,B,C,D,E,F,G,H);
    }
    res=(word*) malloc(sizeof(word)*8);
    *(res)=A;
    *(res+1)=B ;
    *(res+2)=C ;
    *(res+3)=D ;
    *(res+4)=E ;
    *(res+5)=F ;
    *(res+6)=G ;
    *(res+7)=H;
    printf("%02d  %08x %08x %08x %08x %08x %08x %08x %08x \n",-1,*(v)^A,B,C,D,E,F,G,H);
    return  _xor(res,v); ;
}

char* toHex(word* b,int len){
    char *hex;
    hex=(char*)malloc(sizeof(char)*len*8);
    for(int i=0;i<len;i++){
        *(hex+(i*8)+0)=*(hexMap+((*(b+i)>>28)&0xf));
        *(hex+(i*8)+1)=*(hexMap+((*(b+i)>>24)&0xf));
        *(hex+(i*8)+2)=*(hexMap+((*(b+i)>>20)&0xf));
        *(hex+(i*8)+3)=*(hexMap+((*(b+i)>>16)&0xf));
        *(hex+(i*8)+4)=*(hexMap+((*(b+i)>>12)&0xf));
        *(hex+(i*8)+5)=*(hexMap+((*(b+i)>>8)&0xf));
        *(hex+(i*8)+6)=*(hexMap+((*(b+i)>>4)&0xf));
        *(hex+(i*8)+7)=*(hexMap+((*(b+i)>>0)&0xf));
    }
    return hex ;
}

char* sm3(char*msg,int len){
    //dispart
    FillMsg *fm= fill(msg,len);
    printf("fill:");
    for(int i=0;i<64;i++){
        printf("%x",(unsigned char)*(fm->msg+i));

    }
    printf("\n");
    int groupCount=fm->bitSize/512;

    word *iv_0,*iv_1 ,*block;
    iv_0=(word*) malloc(sizeof(word)*8);
    block=(word*) malloc(sizeof(word)*16);
    memcpy(iv_0,IV,sizeof(word)*8);

    for(int i=0;i<groupCount;i++){
        memcpy(block,(fm->msg+(i*(sizeof(word)*16))),sizeof(word)*16);
        for(int j=0;j<16;j++){
            *(block+j)= exchange(*(block+j));
        }
        iv_1= compress(iv_0,block);
        iv_0=iv_1;
    }
    return toHex(iv_1,8);
}
void test(){
    char *p="abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd";
    std::string answer,targetAnswer="debe9ff92275b8a138604889c18e5a4d6fdb70e5387e5765293dcba39c0c5732";
    answer=p;
    const char *q;
    q= sm3(p,64);
    for(int i=0;i<64;i++){
        printf("%c", *(q+i));

    }
    assert(!targetAnswer.compare(q));

}
int main() {
    test();
    return 0;
}
