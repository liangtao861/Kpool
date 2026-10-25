#undef NDEBUG
#include "kpool.h"

using namespace std;

class foo2{
public:
	int a;
	int b;
	friend ostream& operator<<(ofstream& os,foo2 in);
	foo2(int a1,int b1) : a(a1),b(b1) {}
};

ostream& operator<<(ostream& os,foo2 in){
	os << "a:" << in.a << "b:" << in.b << endl;
	return os;
}

bool test1(){
	memoryPool a;
	a.init();
	auto myPtr = a.kmalloc<int>(64);
	try{
		myPtr = myPtr + 39;
	}catch (out_of_range){
		return true;
	}
	return false;
}

bool test2(){
	memoryPool a;
	a.init();
	try{
		
		while(1){
			//cout << "continue..." << endl;
			auto myPtr = a.kmalloc<int>(2049);
		}
	}catch (invalid_argument){
		return true;
	}
	return false;
}

bool test3(){
	memoryPool a;
	try{
		auto ptr = a.kcalloc<int>(100,1);
	}catch (logic_error){
		return true;
	}
	return false;
}

bool test4(){
	memoryPool a;
	a.init();
	auto myPtr = a.kmalloc<int>(4);
	auto aPtr = a.kmalloc<int>(8);
	try{
		myPtr = nullptr;
		myPtr.get(4);
	}catch(std::logic_error){
		return true;
	}
	return false;
	 
}

bool test5(){
	memoryPool a;
	a.init();
	auto myPtr = a.kmalloc<std::string>(128);
	*(myPtr) = "hello";
	cout << *(myPtr) << endl;
	cin >> *(myPtr);
	cout << *(myPtr) << endl;
	return true;
}

bool test6(){
	memoryPool a;
	a.init();
	auto myPtr = a.kmalloc<foo>(sizeof(foo) * 20);
	myPtr[2] = foo(9,8);
	(myPtr + 2)->view();
	return true; 
}

bool test7(){
	memoryPool a;
	a.init();
	auto myPtr = a.kmalloc<int>(sizeof(int) * 64);
	auto otherPtr = myPtr + 63 - 63;
	assert(otherPtr == myPtr);
	return true;
}

bool test8(){
	memoryPool a;
	a.init();
	auto Ptr1 = a.kmalloc<int>(32);
	auto Ptr2 = a.kmalloc<int>(32);
	auto Ptr3 = a.kmalloc<int>(32);
	Ptr2 = nullptr;
	auto testPtr = a.kmalloc<int>(64);
	for(int i=0;i< 16;i++){
		*(testPtr + i) = i;
	}
	auto aPtr = testPtr + 15;
	int c = 0;
	for(int i=15;i >= 0;i--){
		cout << *(aPtr - i) << endl;
		c += *(aPtr - i);
	}
	cout << c << endl;
	assert(c == 120);
	
	aPtr.reset();
	assert(aPtr == testPtr);
	return true;
}

bool test9(){
	auto testMemoryCrt = []() -> int{
		memoryPool c;
		c.init();
		return 0;
	};
	memoryPool a;
	a.init();
	memoryPool b;
	b.init();
	testMemoryCrt();
	memoryPool d;
	d.init();
	return true;
}

bool test10(){
	memoryPool a;
	a.init();
	auto b = a.kmalloc<int>(256);
	{
		auto c = a.kmalloc<int>(64);
		auto f = c;
		cout << "----" << endl;
	}
	{
		auto d = a.kmalloc<int>(1024);
	}
	auto e = a.kmalloc<int>(256);
	return true;
}

bool test11(){
	memoryPool k;
	k.init();
	auto a = k.kmalloc<int>(sizeof(int) * 64);
	auto b = k.kmalloc<int>(sizeof(int) * 64);
	for(int i = 0;i < 64;i++)
		*(a + i) = i * 2;
	int sum = 0;
	for(int i = 0;i < 64;i++){
		cout << *(a + i)  << endl;
		//cout << "-----" << endl;
		*(b + i) = *(a + i);
		cout << *(b + i) << endl;
	}
	for(int i = 0;i < 64;i++){
		cout << *(b + i) << endl;
		sum += *(b + i);
	}
	//cout << sum << endl;
	assert(sum == 4032);
	return true;
}

bool test12(){
	memoryPool a;
	a.init();
	auto ptr = a.kcalloc<std::string>(256,1,0x00);
	cout << ptr[1] << endl;
	ptr[2] = "hello";
	cout << ptr[2] << endl;
	return true;
}

int testForEqual(){
	vector<bool(*)()> equalTest = {
		test1,
		test2,
		test3,
		test4,
		test5,
		test6,
		test7,
		test8,
		test9,
		test10,
		test11,
		test12
	};
	vector<string> equalTestString = {
		"out-range move",
		"out-alloc",
		"Invalid input par",
		"nullptr test",
		"Non-POD store test",
		"self View Test",
		"one-same test",
		"I/O test",
		"memoryPool Destory Test",
		"auto free test",
		"copy test",
		"STL support"
	};
	int _ = 0;
	cout.setf(ios_base::boolalpha);
	for(auto&& get: equalTest){
		cout << (equalTestString[_++]) << "[success:" << get() << "]" << endl; 	
	}
	return 0;
}
int main() {
	testForEqual();
	return 0;
}

// foo2 的显式实例化列表
