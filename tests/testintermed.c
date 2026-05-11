int a;
int a[];
int a[10];
int a[];
struct A { };
struct Pt{
	int x;
	int y;
	};
struct Pt points[10];
int len(char s[]){
	int i;
	i=0;
	while(s[i])i=i+1;
	return i;
	}
double max(double a,double b){
	if(a>b)return a;
		else return b;
	}
int arithmetic(){
	int a;
	int b;
	int c;
	a=10;
	b=3;
	c=a+b*2;
	c=a-b/2;
	c=a*b+1;
	c=(a+b)*2;
	return c;
}
int logic(){
	int a;
	int b;
	a=5;
	b=10;
	if(a<b)return 1;
	if(a<=b)return 1;
	if(a>b)return 0;
	if(a>=b)return 0;
	if(a==b)return 0;
	if(a!=b)return 1;
	if(a<b&&b>0)return 1;
	if(a>0||b>0)return 1;
	if(!a)return 0;
	return 0;
}
int arrayTest(){
	int i;
	struct Pt p;
	i=points[0].x;
	points[1].y=42;
	p.x=1;
	p.y=2;
	return i;
	}
int castTest(){
	double x;
	int n;
	x=3.14;
	n=(int)x;
	x=(double)n;
	n=(int)(x+1.5);
	return n;
	}
int ifTest(int a,int b){
	int r;
	if(a>0){
		if(b>0){
			r=1;
			}
		else{
			r=2;
			}
		}
	else{
		r=3;
		}
	return r;
	}
int whileTest(){
	int i;
	int s;
	i=0;
	s=0;
	while(i<10){
		s=s+i;
		i=i+1;
		}
	return s;
	}
int callTest(){
	int x;
	int y;
	x=max(1,2);
	y=len("hello");
	x=max(x+1,y*2);
	return x;
	}
void voidTest(){
	int i;
	i=0;
	if(i==0)return;
	i=1;
	}
int unaryTest(){
	int a;
	int b;
	a=5;
	b=-a;
	b=!a;
	b=-(-a);
	return b;
	}
int stringTest(){
	char c;
	char s[];
	c='x';
	c='\n';
	return 0;
	}
void main(){
	int i;
	i=10;
	while(i!=0){
		puti(i);
		i=i/2;
		}
    }