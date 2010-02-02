class ClassA {

	ClassB b;
	int a;

	public ClassA(int i) {
		a = i;
	}

	public ClassA(){
		b = new ClassB();
		a = b.getInt(3);
	};

	public int getInt(int i) {
		return i*2;
	}

	public void printInt() {
		System.out.println(a);
	}
}

