class ClassB {

	ClassA a;
	int b;

	public ClassB(){
		a = new ClassA(1);
		b = a.getInt(2);
	};

	public int getInt(int i) {
		return i*2;
	}

	public void printInt() {
		System.out.println(b);
	}
}

