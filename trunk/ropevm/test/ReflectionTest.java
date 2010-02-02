
import java.lang.*;
import java.lang.reflect.*;

class ReflectionTest{
	private String foo;

	private ReflectionTest(){
		System.out.println("In nullary constructor");
		this.foo = "No arguments";
	}

	private ReflectionTest(String foo){
		System.out.println("In string constructor(" + foo + ")");
		this.foo = foo;
	}

	public static void constructor(){
		System.out.println("*** Test: constructor");
		try{
			Class me = Class.forName("ReflectionTest");
			System.out.println(me.getName());
			Class sup = me.getSuperclass();
			System.out.println(sup.getName());
			Object ob = new ReflectionTest();
			System.out.println((me.isInstance(ob))? "isInstance" : "NOT INSTANCE");
			Class me2 = ob.getClass();
			System.out.println((me==me2)? "Equal" : "UNEQUAL");
			System.out.println((sup.isAssignableFrom(me))? "assignable" : "NOT ASSIGNABLE");
			System.out.println((me.isAssignableFrom(sup))? "ASSIGNABLE" : "not assignable");
			System.out.println(me.getModifiers());
			Class string = Class.forName("java.lang.String");
			System.out.println(string.getName());
			Class emptybench = Class.forName("EmptyBench");
			System.out.println(emptybench.getName());
			// TODO: Try a class that doesn't exist
			Constructor ctor = me.getDeclaredConstructor(new Class[0]);
			System.out.println(ctor.toString());
			ReflectionTest instance = (ReflectionTest)ctor.newInstance(new Object[0]);
			if(instance == null)
				System.out.println("ERROR: newInstance returned null");
			else
				System.out.println(instance.foo);
		}catch(Throwable e){
			System.out.println("EXCEPTION: " + e);
		}
	}

	public static void constructor2(){
		System.out.println("*** Test: constructor2");
		try{
			Class me = Class.forName("ReflectionTest");
			Class[] argClasses = new Class[1];
			argClasses[0] = Class.forName("java.lang.String");
			Constructor ctor = me.getDeclaredConstructor(argClasses);
			//System.out.println(ctor.toString());
			//This doesn't yet print the parameters properly
			Object[] args = new Object[1];
			args[0] = "It worked";
			ReflectionTest instance = (ReflectionTest)ctor.newInstance(args);
			if(instance == null)
				System.out.println("ERROR: newInstance returned null");
			else
				System.out.println(instance.foo);
		}catch(Throwable e){
			System.out.println("EXCEPTION: " + e);
		}
	}

	public static void field(){
		System.out.println("*** Test: field");
		ReflectionTest rt = new ReflectionTest("Field test #1");
		try{
			Field fd = rt.getClass().getDeclaredField("foo");
			System.out.println(fd.getName());
			System.out.println(fd.getType().getName());
			System.out.println((String)fd.get(rt));
			fd.set(rt, "Field test #2");
			System.out.println((String)fd.get(rt));
			System.out.println(rt.foo);
		}catch(NoSuchFieldException e){
			System.out.println("ERROR: NoSuchFieldException");
		}catch(IllegalAccessException e){
			System.out.println("ERROR: IllegalAccessException");
		}catch(Throwable e){
			System.out.println("ERROR: Unexpected exception: " + e.toString());
		}
		try{
			Field fd = rt.getClass().getDeclaredField("nonexistant");
			System.out.println("NO EXCEPTION THROWN FOR NONEXISTANT FIELD");
		}catch(NoSuchFieldException e){
			System.out.println("Ok: NoSuchFieldException");
		}catch(Throwable e){
			System.out.println("ERROR: Unexpected exception: " + e.toString());
		}
	}

	public static void main(String[] cmdlineArgs){
		constructor();
		constructor2();
		field();
	}

}

