struct S{
	int n;
	char text[16];
	};
	
struct S a;
struct S v[10];

void f(char text[],int i,char ch){
	text[i]=ch;
	}

int h(int x,int y){
	if(x>0&&x<y){
		f(v[x].text,y,'#');
		return 1;
		}
	return 0;
	}



// IF - conditia trebuie sa fie scalar
void ifTest() {
	//if (v) {}
}

// WHILE - conditia trebuie sa fie scalar
void whileTest() {
	//while (v) {}
}
// RETURN - expresia trebuie sa fie scalar
int scalarReturnTest(){
	//return v;
}

// RETURN - functiile void nu pot returna o valoare

void voidReturnTest(){
//	return 5;
}


// RETURN - functiile non-void trebuie sa aiba o expresie returnata, a carei tip sa fie convertibil la tipul returnat de functie
int returnTypeTest(){
	//return;
	//return a;
}

// Destinatia trebuie sa fie left-value
void leftValueTest(){
	//5 = a;
}

// Destinatia nu trebuie sa fie constanta
void constantTest(){
	//a.text = 5;
}

// Ambii operanzi trebuie sa fie scalari
void scalarOperandsTest(){
	//a =  v;
}

// Sursa trebuie sa fie convertibila la destinatie
void sursaConvertibilaTest(){
	//a = 5;
}

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
void bothOperandsScalarsNotStruct(){
	int b;
	//b = a || a;
	//b = a && a;
	//a == 3;
	//a != 4;
	//a > 4;
	//a >= 4;
	//a < 4;
	//a <= 4;
	//a + 4;
	//a - 4;
	//a*4;
	//a/4;
}

// Structurile nu se pot converti
void cantConvertStruct(){
	//(int)a;
}
// Tipul la care se converteste nu poate fi structura
void cantConvertToStruct(){
	//(struct S)a;
}

// Un array se poate converti doar la alt array
void arrayOnlyConvertsArray(){
	int b[20];
	//(double)b;
}

// Un scalar se poate converti doar la alt scalar
void scalarToScalar(){
	int b;
	//(double[])b;
}

// Minus unar si Not trebuie sa aiba un operand scalar
void unaryMustHaveScalarOperand(){
	//-v;
	//!v;
}

// Doar un array poate fi indexat
void indexNonArray(){
	//a[2];
}

// Indexul in array trebuie sa fie convertibil la int
void intIndex(){
	//v[a];
}

// Operatorul de selectie a unui camp de structura se poate aplica doar structurilor
void selectNonStruct(){
	int b;
	//b.n;
}

// Campul unei structuri trebuie sa existe
void fieldMustExist(){
	//a.asd;
}

// ID-ul trebuie sa existe in TS
void idMustExist(){
	//b + 4;
}
// Doar functiile pot fi apelate
void callNonFunction(){
	//a(2);
}

// O functie poate fi doar apelata
void functionCanOnlyBeCalled(){
	//functionCanOnlyBeCalled + 2;
}

// Apelul unei functii trebuie sa aiba acelasi numar de argumente ca si numarul de parametri de la definitia ei
void sameParameterCountFunction(int b){
	//sameParameterCountFunction();
	//sameParameterCountFunction(2,3)
}

// Tipurile argumentelor de la apelul unei functii trebuie sa fie convertibile la tipurile parametrilor functiei
void compatibleParameters(int b){
	//compatibleParameters(a);
}

	