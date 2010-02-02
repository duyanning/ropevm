
class InstanceTest{
	int i; double d;

	InstanceTest(int i, double d){
		this.i = i;
		this.d = d;
	}

	public void print(){
		Util.printIntAndDouble(i,d);
	}

	public static void main(String[] args){
		InstanceTest it = new InstanceTest(373, 374.375);
		it.print();
	}
}

