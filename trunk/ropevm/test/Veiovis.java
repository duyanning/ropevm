
import java.lang.Cloneable;
import java.io.Serializable;

/* Veiovis, the god of healing */

class Veiovis implements Cloneable{

	/* The "->" comments indicate ONLY those bytecodes whose results
	   are verified by each operation; not all the bytecodes executed.
	*/

	static void print(String  x){ System.out.println(x); }
	static void print(boolean x){ System.out.println(x); }
	static void print(char    x){ System.out.println(x); }
	static void print(int     x){ System.out.println(x); }
	static void print(long    x){ System.out.println(x); }
	static void print(Object  x){ System.out.println(x); }

	/* Printing of floating-point numbers in GNU Classpath isn't
	   100% Java compliant.  Anywhere this has affected the test
	   suite has been marked with an "FPRINT" marker comment.
	*/

	private static final double ln2 = Math.log(2);
	private static final double ln10 = Math.log(10);
	private static final double digitsPerBit = ln2/ln10;

	/* FPRINT */
	static void print(float   x){ print((double)x); }
	static void print(double  x){
		StringBuffer str = new StringBuffer("");
		if(Double.isNaN(x)){
			str.append("NaN");
		}else{
			if(x < 0)
				str.append("-");
			if(Double.isInfinite(x)){
				str.append("Infinity");
			}else if(x == 0){
				str.append("0.0");
			}else{
				/* TODO: Handle gradual underflow? */
				long bits = Double.doubleToLongBits(x);
				long log2x = ((bits & 0x7ff0000000000000L) >> 52) - 1023;
				int log10x = (int)(log2x * digitsPerBit);
				double pow10 = Math.exp(log10x * ln10);
				double norm = x / pow10;
				System.out.println("Norm: " + Double.toString(norm));
				/* Now norm is between 1 and 10 */
				long normBits = Double.doubleToLongBits(norm);
				long normExp = ((normBits & 0x7ff0000000000000L) >> 52) - 1023;
				long placeVal = 0x0010000000000000L; /* Mantissa for 1.0 */
				System.out.println("placeVal: " + Long.toString(placeVal));
				long mant = ((normBits & 0x000fffffffffffffL) + placeVal) << normExp;
				/* Do the digits */
				int digit = (int)(mant/placeVal);
				str.append(Integer.toString(digit) + ".");
				mant -= digit * placeVal;
				placeVal /= 10;
				do{
					digit = (int)(mant/placeVal);
					str.append(Integer.toString(digit));
					mant -= digit * placeVal;
					placeVal /= 10;
				}while(mant != 0 && placeVal != 0);
				/* Do the exponent */
				if(log2x != 0){
					str.append("E" + Long.toString(log10x));
				}
			}
			print(str.toString());
		}
	}

	private static void aloadStore(){
		Object o0,o1,o2,o3,o4;
		o0 = "0"; //-> ASTORE_0
		o1 = "1"; //-> ASTORE_1
		o2 = "2"; //-> ASTORE_2
		o3 = "3"; //-> ASTORE_3
		o4 = "4"; //-> ASTORE
		print(o0); //-> ALOAD_0
		print(o1); //-> ALOAD_1
		print(o2); //-> ALOAD_2
		print(o3); //-> ALOAD_3
		print(o4); //-> ALOAD
	}

	private static void dloadStore(){
		double d0,d1,d2,d3,d4;
		d0 = 0; //-> DSTORE_0
		d1 = 1; //-> DSTORE_1
		d2 = 2; //-> DSTORE_2
		d3 = 3; //-> DSTORE_3
		d4 = 4; //-> DSTORE
		//FPRINT
		print(d0==0); //-> DLOAD_0
		print(d1==1); //-> DLOAD_1
		print(d2==2); //-> DLOAD_2
		print(d3==3); //-> DLOAD_3
		print(d4==4); //-> DLOAD
	}

	private static void floadStore(){
		float f0,f1,f2,f3,f4;
		f0 = 0; //-> FSTORE_0
		f1 = 1; //-> FSTORE_1
		f2 = 2; //-> FSTORE_2
		f3 = 3; //-> FSTORE_3
		f4 = 4; //-> FSTORE
		//FPRINT
		print(f0==0); //-> FLOAD_0
		print(f1==1); //-> FLOAD_1
		print(f2==2); //-> FLOAD_2
		print(f3==3); //-> FLOAD_3
		print(f4==4); //-> FLOAD
	}

	private static void iloadStore(){
		int i0,i1,i2,i3,i4;
		i0 = 0; //-> ISTORE_0
		i1 = 1; //-> ISTORE_1
		i2 = 2; //-> ISTORE_2
		i3 = 3; //-> ISTORE_3
		i4 = 4; //-> ISTORE
		print(i0); //-> ILOAD_0
		print(i1); //-> ILOAD_1
		print(i2); //-> ILOAD_2
		print(i3); //-> ILOAD_3
		print(i4); //-> ILOAD
	}

	private static void lloadStore(){
		long l0,l1,l2,l3,l4;
		l0 = 0; //-> LSTORE_0
		l1 = 1; //-> LSTORE_1
		l2 = 2; //-> LSTORE_2
		l3 = 3; //-> LSTORE_3
		l4 = 4; //-> LSTORE
		print(l0); //-> LLOAD_0
		print(l1); //-> LLOAD_1
		print(l2); //-> LLOAD_2
		print(l3); //-> LLOAD_3
		print(l4); //-> LLOAD
	}

	public static void loadStore(){
		print("loadStore");
		int    i1,i2,i3,i4;
		long   l1,l2,l3,l4;
		float  f1,f2,f3,f4;
		double d1,d2,d3,d4;
		Object o1,o2,o3,o4;
		/* Int */
		//-> BIPUSH
		//-> SIPUSH
		//-> LDC
		print(-2);
		print(-1); //-> ICONST_M1
		print(0);  //-> ICONST_0
		print(1);  //-> ICONST_1
		print(2);  //-> ICONST_2
		print(3);  //-> ICONST_3
		print(4);  //-> ICONST_4
		print(5);  //-> ICONST_5
		print(6);
		print(127); print(-127);
		print(128); print(-128);
		print(129); print(-129);
		print(32767); print(-32767);
		print(32768); print(-32768);
		print(32769); print(-32769);
		print(2147483647); print(-2147483647); print(-2147483648);
		/* Long */
		//-> LDC2W
		//-> LLOAD
		print(-1L);
		print(0L); //-> LCONST_0 
		print(1L); //-> LCONST_1
		print(2L);
		print(0x7FFFFFFFFFFFFFFFL);
		print(0x8000000000000000L);
		/* Float */
		/* Double */
		/* Reference */
		print(null);      //-> ACONST_NULL
		o1 = System.out;  //-> ALOAD
		if(o1 == null) print("Whoops, null"); else print("not null");
		/* Various load/store opcodes */
		aloadStore();
		dloadStore();
		floadStore();
		iloadStore();
		lloadStore();
	}

	private static void ibitwise(int value, int mask){
		print(value & mask); //-> IAND
		print(mask & value);
		print(value ^ mask); //-> IXOR
		print(mask ^ value);
		print(value | mask); //-> IOR
		print(mask | value);
	}

	private static void lbitwise(long value, long mask){
		print(value & mask); //-> LAND
		print(mask & value);
		print(value | mask); //-> LOR
		print(mask | value);
		print(value ^ mask); //-> LXOR
		print(mask ^ value);
	}

	public static void bitwise(){
		print("bitwise-int");
		int i1 = 0x0124569A;
		ibitwise(i1, ~i1);
		ibitwise(i1, 0);
		ibitwise(i1, -1);
		ibitwise(i1, 0x55555555);
		ibitwise(i1, 0xAAAAAAAA);
		ibitwise(i1, 0xA5A55A5A);
		ibitwise(i1, 0x5A5AA5A5);
		ibitwise(i1, 0xDEADBEEF);
		print("bitwise-long");
		long l1 = 0x0124569ADEADBEEFL;
		lbitwise(l1, ~i1);
		lbitwise(l1, 0);
		lbitwise(l1, -1);
		lbitwise(l1, 0x5555555555555555L);
		lbitwise(l1, 0xAAAAAAAAAAAAAAAAL);
		lbitwise(l1, 0x5A5AA5A5A5A55A5AL);
		lbitwise(l1, 0xA5A55A5A5A5AA5A5L);
		lbitwise(l1, 0xDEADBEEF4215600DL);
	}

	public static void arith(){
		print("arith");
		int    i1,i2,i3,i4;
		long   l1,l2,l3,l4;
		float  f1,f2,f3,f4;
		double d1,d2,d3,d4;
		/* Add/subtract */
		//-> IADD
		//-> ISUB
		i1 = 1;
		i2 = -2;
		i3 = 2147483647;
		print(i1+i2);
		print(i3+i1);
		print(i3+i2);
		print(i1-i2);
		print(i1-i3);
		print(i2-i3);
		//-> LADD
		//-> LSUB
		l1 = 1;
		l2 = -2;
		l3 = 2147483647;
		print(l1+l2);
		print(l3+l1);
		print(l3+l2);
		print(l1-l2);
		print(l1-l3);
		print(l2-l3);
		l3 = 0x7FFFFFFFFFFFFFFFL;
		print(l3);
		print(l1+l3);
		// FPRINT
		/*
		f1 = (float)Math.PI;
		print(f1);
		print(f1+f1);
		d1 = Math.PI;
		print(d1);
		print(d1+d1);
		*/
		/* Multiply */
		//-> IMUL
		i1 = 123;
		i2 = 100000;
		i3 = -100000;
		print(i1*i2);
		print(i2*i2);
		print(i1*i3);
		print(i3*i3);
		print(i2*i3);
		//-> LMUL
		l1 = 123;
		l2 = 100000;
		l3 = -100000;
		print(l1*l2);
		print(l2*l2);
		print(l1*l3);
		print(l3*l3);
		print(l2*l3);
		print(l3*l2);
		l2 = l2 * l2;
		l3 = l2 * l3;
		print(l2*l2);
		print(l3*l3);
		print(l3*l2);
		print(l2*l3);
		/* Divide, remainder */
		//-> IDIV
		//-> IREM
		i1 = 2;
		i2 = 3;
		i3 = -2;
		i4 = -3;
		print(i1/i1); print(i1%i1);
		print(i1/i2); print(i1%i2);
		print(i1/i3); print(i1%i3);
		print(i1/i4); print(i1%i4);
		print(i2/i1); print(i2%i1);
		print(i2/i2); print(i2%i2);
		print(i2/i3); print(i2%i3);
		print(i2/i4); print(i2%i4);
		print(i3/i1); print(i3%i1);
		print(i3/i2); print(i3%i2);
		print(i3/i3); print(i3%i3);
		print(i3/i4); print(i3%i4);
		print(i4/i1); print(i4%i1);
		print(i4/i2); print(i4%i2);
		print(i4/i3); print(i4%i3);
		print(i4/i4); print(i4%i4);
		try{
			i1 = 0;
			print(5/i1);
		}catch(ArithmeticException e){
			print("Caught ArithmeticException");
		}
		try{
			i1 = 0;
			print(5%i1);
		}catch(ArithmeticException e){
			print("Caught ArithmeticException");
		}
		//-> LDIV
		//-> LREM
		l1 = 2;
		l2 = 3;
		l3 = -2;
		l4 = -3;
		print(l1/l1); print(l1%l1);
		print(l1/l2); print(l1%l2);
		print(l1/l3); print(l1%l3);
		print(l1/l4); print(l1%l4);
		print(l2/l1); print(l2%l1);
		print(l2/l2); print(l2%l2);
		print(l2/l3); print(l2%l3);
		print(l2/l4); print(l2%l4);
		print(l3/l1); print(l3%l1);
		print(l3/l2); print(l3%l2);
		print(l3/l3); print(l3%l3);
		print(l3/l4); print(l3%l4);
		print(l4/l1); print(l4%l1);
		print(l4/l2); print(l4%l2);
		print(l4/l3); print(l4%l3);
		print(l4/l4); print(l4%l4);
		try{
			l1 = 0;
			print(5/l1);
		}catch(ArithmeticException e){
			print("Caught ArithmeticException");
		}
		try{
			l1 = 0;
			print(5%l1);
		}catch(ArithmeticException e){
			print("Caught ArithmeticException");
		}
	}

	private static void intShiftPrint(int value, int shift){
		print("intShiftPrint(" + value + ", " + shift + ")");
		//-> ISHL
		//-> ISHR
		//-> IUSHR
		print(value << shift);
		print(value >> shift);
		print(value >>> shift);
		value = -value;
		print(value << shift);
		print(value >> shift);
		print(value >>> shift);
	}

	private static void longShiftPrint(long value, int shift){
		print("longShiftPrint(" + value + ", " + shift + ")");
		//-> LSHL
		//-> LSHR
		//-> LUSHR
		print(value << shift);
		print(value >> shift);
		print(value >>> shift);
		value = -value;
		print(value << shift);
		print(value >> shift);
		print(value >>> shift);
	}

	public static void shift(){
		print("shift");
		int    i1,i2,i3,i4;
		long   l1,l2,l3,l4;
		i1 = 0x4000005;
		intShiftPrint(i1,1);
		intShiftPrint(i1,2);
		intShiftPrint(i1,29);
		intShiftPrint(i1,31);
		intShiftPrint(i1,32);
		intShiftPrint(i1,33);
		l1 = 0x400000006000005L;
		longShiftPrint(l1,1);
		longShiftPrint(l1,2);
		longShiftPrint(l1,29);
		longShiftPrint(l1,31);
		longShiftPrint(l1,32);
		longShiftPrint(l1,33);
		longShiftPrint(l1,61);
		longShiftPrint(l1,63);
		longShiftPrint(l1,64);
		longShiftPrint(l1,65);
	}

	public static void array(){
		print("array");
		/* We also do booleans here, not to test another bytecode,
		   but just to make sure boolean arrays work, because they
		   may be handled differently internally.
		*/
		//-> NEWARRAY
		boolean booleans[] = new boolean[10];
		byte bytes[] = new byte[10];
		char chars[] = new char[10];
		double doubles[] = new double[10];
		float floats[] = new float[10];
		int ints[] = new int[10];
		long longs[] = new long[10];
		Object objects[] = new Object[10]; //-> ANEWARRAY
		Object arrays[][] = new Object[1][];
		//TODO: Test multidimensional arrays
		for(int i=0; i < 10; i++){
			booleans[i] = ((i & 1) == 0);
			objects[i] = new Integer(i); //-> AASTORE
			bytes[i] = (byte)i;          //-> BASTORE
			chars[i] = (char)('A'+i);    //-> CASTORE
			doubles[i] = i;              //-> DASTORE
			floats[i] = i;               //-> FASTORE
			ints[i] = i;                 //-> IASTORE
			longs[i] = i;                //-> LASTORE
		}
		//-> ARRAYLENGTH
		print(booleans.length);
		print(objects.length);
		print(bytes.length);
		print(chars.length);
		print(doubles.length);
		print(floats.length);
		print(ints.length);
		print(longs.length);
		for(int i=-1; i < 11; i++){
			try{
				print(booleans[i]);
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				print(objects[i]); //-> AALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				print(bytes[i]); //-> BALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				print(chars[i]); //-> CALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				// FPRINT
				print(doubles[i] == i); //-> DALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				// FPRINT
				print(floats[i] == i); //-> FALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				print(ints[i]); //-> IALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
			try{
				print(longs[i]); //-> LALOAD
			}catch(ArrayIndexOutOfBoundsException e){
				print("Out of bounds");
			}
		}
	}

	private static void xthrow(Throwable x) throws Throwable{
		try{
			throw x; //-> ATHROW
		}catch(NullPointerException e){
			print(e.toString() + " caught in xthrow");
			throw e;
		}finally{ //-> GOTO
			print(x.toString() + " finally");
		}
	}

	public static void exception(){
		print("exception");
		try{
			xthrow(new NullPointerException());
		}catch(Throwable e){
			print(e.toString() + " caught in exception");
		}
		try{
			xthrow(new OutOfMemoryError());
		}catch(Throwable e){
			print(e.toString() + " caught in exception");
		}
	}

	private static void voidReturn(boolean flag){
		if(flag) return;
		print("Whoops");
	}

	private static String areturn(boolean flag){
		if(flag) return "ok"; //-> ARETURN
		print("Whoops");
		return "Whoops";
	}

	private static double dreturn(boolean flag){
		if(flag) return Math.PI; //-> DRETURN
		print("Whoops");
		return 0;
	}

	private static float freturn(boolean flag){
		if(flag) return (float)123.456; //-> FRETURN
		print("Whoops");
		return 0;
	}

	private static int ireturn(boolean flag){
		if(flag) return 0xCAFEBABE; //-> IRETURN
		print("Whoops");
		return 0;
	}

	private static long lreturn(boolean flag){
		if(flag) return 0xDEADBEEF4215600DL; //-> LRETURN
		print("Whoops");
		return 0;
	}

	public static void returns(){
		print("returns");
		voidReturn(true);
		print(areturn(true));
		print(dreturn(true) == Math.PI);        //FPRINT
		print(freturn(true) == (float)123.456); //FPRINT
		print(ireturn(true));
		print(lreturn(true));
	}

	private static void switchTest(int value){
		/* Currently, this first TableSwitch starts at an address
		   which is divisible by 4.  If you change the code, try
		   to leave at least one of the switch statements on such
		   an address, since that will test the interpreter's
		   handling of the zero-padding in the xxxSWITCH opcodes.
		*/
		print(value);
		//-> TABLESWITCH
		switch(value){
		case 2:
			print("Two");
			break;
		case 3:
			print("Three");
			break;
		default:
			print("Not two or three");
			break;
		}
		/* Lookupswitch can be implemented using a binary search,
		   and often off-by-one errors in binary searches only turn
		   up with either even or odd numbers of items, but not both.
		   Thus, we test both cases.
		*/
		//-> LOOKUPSWITCH
		// Even number of cases
		switch(value){
		case 2:
			print("Two again");
			break;
		case 3:
			print("Three again");
			break;
		case 102:
			print("One hundred and two");
			break;
		case 0x7FFFFFFF:
			/* This big number is important, because if the binary
			   search overshoots the last case, it will be reaching into
			   the bytecodes after the switch statement.  Whatever those
			   bytecodes are, we want the binary search to continue to
			   run off into la-la land so it is more likely never to
			   come back and find the right case despite the bug.  Thus,
			   the number it's looking for should be bigger than anything
			   it could find in random bytecodes.
			*/
			print("Really big positive");
			break;
		default:
			print("Not 2 or 3 or 102 or really big");
			break;
		}
		// Odd number of cases
		switch(value){
		case 3:
			print("Three again");
			break;
		case 103:
			print("One hundred and three");
			break;
		case -1000000003: /* Make sure multi-byte negative values are handled properly */
			print("Minus one billion and three");
			break;
		default:
			print("Not 3, 103, or -1000000003");
			break;
		}
	}

	public static void switches(){
		/* Make sure the default case is handled cleanly for values
		   just outside the tableswitch's range:
		*/
		print("switches");
		switchTest(1);
		switchTest(2);
		switchTest(3);
		switchTest(4);
		/* Test the lookupswitches: */
		switchTest(100);
		switchTest(102);
		switchTest(103);
		switchTest(1000);
		switchTest(0x7FFFFFFF);
		switchTest(-1000000003);
	}

	private static void conformanceImpl(Object sub){
		try{ //-> CHECKCAST
			Cloneable c = (Cloneable)sub;
			print(c == sub);
			print("Arrays are Cloneable");
			Serializable s = (Serializable)sub;
			print(s == sub);
			print("Arrays are Serializable");
			Object[] obs = (Object[])sub;
			print(obs == sub);
			print("Arrays conform to Object[]");
			Cloneable[] sup = (Cloneable[])sub;
			print(sup == sub);
			print("Arrays conform to super-arrays");
			Object[][] obss = (Object[][]) sub;
			print(obss == sub);
			print("This one conforms to Object[][]");
		}catch(ClassCastException e){
			print("Doesn't conform");
		}
	}

	public static void conformance(){
		conformanceImpl(new Veiovis[1]);
		conformanceImpl(new Veiovis[1][1]);
	}

	private int iinst1, iinst2;
	private long linst1, linst2;
	private static int istatic;
	private static long lstatic;

	private void iprint(int i, String s){
		print(i);
		print(s);
	}

	private void lprint(long l, String s){
		print(l);
		print(s);
	}

	public static void dup(){
		Veiovis v = new Veiovis();
		int i1;
		long l1;
		String s;
		v.iinst2 = 1+(v.iinst1 = 1000000000);           //-> DUP_X1
		v.linst2 = 1+(v.linst1 = 1010000000000000010L); //-> DUP2_X1
		i1 = 1+(istatic = 1020000020);                  //-> DUP
		l1 = 1+(lstatic = 1030000000000000030L);        //-> DUP2
		print(i1);
		print(l1);
		print(v.iinst1);
		print(v.linst1);
		print(v.iinst2);
		print(v.linst2);
		print(istatic);
		print(lstatic);
		/* TODO: DUP_X2 and DUP2_X2 */
	}

	public static void runTests(){
		loadStore();
		bitwise();
		arith();
		shift();
		array();
		exception();
		returns();
		switches();
		conformance();
		dup();
	}

	public static void doublePrintTest(){
		print((double)1.0);
		print((double)0.1);
		print((double)100);
		print((double)1e10);
		print((double)Math.PI);
	}

	public static void main(String[] args){
		runTests();
	}

}

