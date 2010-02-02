
interface InitInterface{
	public static int valueFromInterface = new InitTest(123).value;
}

class InitTest implements InitInterface{
	public int value;
	public InitTest(int value){
		this.value = value;
	}

	public static void main(String[] args){
		
		System.out.println(valueFromInterface);
	}
}

