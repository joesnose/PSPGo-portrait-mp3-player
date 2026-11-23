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
 Marquee once={0},loop={0};int finished=0,wrapped=0,prev=0;
 for(unsigned t=0;t<30000000;t+=100000){int x=marquee_once(&once,"once",1,300,200,t);assert(x>=0&&x<=100);if(finished)assert(x==100);if(x==100)finished=1;
 int y=marquee_loop(&loop,"loop",2,300,200,50,t);assert(y>=0&&y<=350);if(y<prev)wrapped++;prev=y;}
 assert(finished&&wrapped>=2);assert(!marquee_once(&once,"short",3,20,200,30000000));assert(!marquee_loop(&loop,"short",3,20,200,50,30000000));
 puts("PASS: pause, bounded movement, end dwell/read completion, selection and text reset, short labels, hidden-view reset timer wrap, one-pass stopping and continuous loop wrap");
}
