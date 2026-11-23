#include "marquee.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 Marquee m={0};assert(!marquee_offset(&m,"long",1,300,200,0));
 for(unsigned t=100000;t<700000;t+=100000)assert(!marquee_offset(&m,"long",1,300,200,t));
 int last=0;for(unsigned t=700000;t<=3800000;t+=100000){int x=marquee_offset(&m,"long",1,300,200,t);assert(x>=last&&x<=100);last=x;}
 assert(marquee_offset(&m,"long",1,300,200,3825000)==100);assert(!marquee_read(&m,"long",1,3825000));
 for(unsigned t=3900000;t<4800000;t+=100000)marquee_offset(&m,"long",1,300,200,t);
 assert(marquee_read(&m,"long",1,4799999));assert(!marquee_read(&m,"other",1,4799999));
 assert(!marquee_offset(&m,"long",2,300,200,4800000));assert(!marquee_offset(&m,"short",3,50,200,4900000));
 assert(!marquee_offset(&m,"long",4,300,200,0xfffffff0));assert(!marquee_offset(&m,"long",4,300,200,0x100));
 for(unsigned t=100000;t<10000000;t+=100000){int x=marquee_offset(&m,"long",4,300,200,t);assert(x>=0&&x<=100);}
 assert(!marquee_offset(&m,"long",4,300,200,12000000));
 puts("PASS: pause, bounded movement, end dwell/read completion, selection and text reset, short labels, hidden-view reset and timer wrap");
}
