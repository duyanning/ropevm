
package java.lang; /* getClassContext is protected */

class ClassContextTest{

	private static void printClass(Class c){
		if(c==null){
			System.out.println("(null)");
		}else{
			System.out.println(c.getName());
		}
	}

	private static void printContext(Class[] context){
		System.out.println("Context has " + context.length + " frames");
		for(int idx=0; idx < context.length; idx++){
			printClass(context[idx]);
		}
	}

	private static Class[] getContext(){
		return new SecurityManager().getClassContext();
	}

	public static void main(String[] args){
		Class[] context = getContext();
		printContext(context);
	}

}

