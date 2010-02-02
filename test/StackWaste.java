
import java.lang.reflect.*;

class StackWaste{

	public static void dummy(
		Object x0 , Object x1 , Object x2 , Object x3 , Object x4 ,
		Object x5 , Object x6 , Object x7 , Object x8 , Object x9 ,
		Object x10, Object x11, Object x12, Object x13, Object x14,
		Object x15, Object x16, Object x17, Object x18, Object x19,
		Object x20, Object x21, Object x22, Object x23, Object x24,
		Object x25, Object x26, Object x27, Object x28, Object x29,
		Object x30, Object x31, Object x32, Object x33, Object x34,
		Object x35, Object x36, Object x37, Object x38, Object x39,
		Object x40, Object x41, Object x42, Object x43, Object x44,
		Object x45, Object x46, Object x47, Object x48, Object x49,
		Object x50, Object x51, Object x52, Object x53, Object x54,
		Object x55, Object x56, Object x57, Object x58, Object x59,
		Object x60, Object x61, Object x62, Object x63, Object x64,
		Object x65, Object x66, Object x67, Object x68, Object x69,
		Object x70, Object x71, Object x72, Object x73, Object x74,
		Object x75, Object x76, Object x77, Object x78, Object x79,
		Object x80, Object x81, Object x82, Object x83, Object x84,
		Object x85, Object x86, Object x87, Object x88, Object x89,
		Object x90, Object x91, Object x92, Object x93, Object x94,
		Object x95, Object x96, Object x97, Object x98, Object x99
	){}
	
	public static void main(String[] cmdline){
		try{
			Class[] argTypes = new Class[100];
			Class objectClass = Class.forName("java.lang.Object");
			for(int i=0; i < 100; i++){
				argTypes[i] = objectClass;
			}
			Method m = Class.forName("StackWaste").getMethod("dummy", argTypes);
			Object[] args = new Object[100];
			for(int i=0; i < 10000; i++){
				m.invoke(null, args);
			}
			System.out.println("Passed.");
		}catch(ClassNotFoundException e){
			System.out.println("CLASS NOT FOUND");
		}catch(NoSuchMethodException e){
			System.out.println("NO SUCH METHOD");
		}catch(IllegalAccessException e){
			System.out.println("ILLEGAL ACCESS");
		}catch(InvocationTargetException e){
			System.out.println("INVOCATION TARGET EXCEPTION");
		}
	}
}

