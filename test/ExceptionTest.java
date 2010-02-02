
class ExceptionTest{

	static void func1(int x, int y, int z){
		throw new InternalError("Testing");
	}

	public static void main(String[] args){
		try{
			func1(1, 2, 3);
			try{
				System.out.println("SHOULDN'T BE HERE");
			}catch(InternalError e){
				System.out.println("Caught it in the WRONG PLACE");
			}
		}catch(InternalError e){
			System.out.println("Caught it");
		}
	}
}

