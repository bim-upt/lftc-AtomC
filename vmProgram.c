f(2.0);
void f(double n){
    double i=0.0;
    while(i<n){
        put_d(i);
        i=i+0.5;
    }
}



	n	 		ret_addr	old_FP	i
        -2	 		-1		0	1	




        PUSH.d 2
        CALL f  //f(2)
        HALT

 
 
f:      ENTER 1 //i

        PUSH.d 0.0
        FPSTORE 1 // i = 0

while:  FPLOAD 1 
        FPLOAD -2
        LESS.d 
        JF done //while i < n

        FPLOAD 1
        CALL put_d //put_d(i)

        FPLOAD 1
        PUSH.d 0.5 
        ADD.d
        FPSTORE 1 //i+=0.5
        JMP while

done:   RET_VOID 1  //return void, get rid of i