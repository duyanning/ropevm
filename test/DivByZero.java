
class DivByZero{
	public static int zero=0;
	public static void main(String[] args){
		try{
			int x = 1/zero;
			System.out.println("NO DIVISION BY ZERO EXCEPTION");
		}catch(ArithmeticException e){
			System.out.println("Caught division by zero exception");
		}
	}
}

