
/* Produces a class file that has all the opcodes */
/* Still missing these:
	19: LDC2
	90: DUP_X1
	91: DUP_X2
	92: DUP2
	93: DUP2_X1
	94: DUP2_X2
	95: SWAP
	124: IUSHR
	125: LUSHR
	186 (undefined)
	196: WIDE
	200: GOTO_W
	201: JSR_W
	202: BREAKPOINT
*/

class AllCodes extends Exception implements Runnable{

	public void run(){}

	public static void iload(int i0, int i1, int i2, int i3, int i4){
		i0=i0;
		i1=i1;
		i2=i2;
		i3=i3;
		i4=i4;
	}

	public static void lload_even(long l0, long l2, long l4){
		l0=l0;
		l2=l2;
		l4=l4;
	}

	public static void lload_odd(int x, long l1, long l3){
		l1=l1;
		l3=l3;
	}

	public static void fload(float f0, float f1, float f2, float f3, float f4){
		f0=f0;
		f1=f1;
		f2=f2;
		f3=f3;
		f4=f4;
	}

	public static void dload_even(double d0, double d2, double d4){
		d0=d0;
		d2=d2;
		d4=d4;
	}

	public static void dload_odd(int x, double d1, double d3){
		d1=d1;
		d3=d3;
	}

	public static void aload(Object a0, Object a1, Object a2, Object a3, Object a4){
		a0=a0;
		a1=a1;
		a2=a2;
		a3=a3;
		a4=a4;
	}

	public static void xload(){
		iload(0,0,0,0,0);
		lload_even(0,0,0);
		lload_odd(0,0,0);
		fload(0,0,0,0,0);
		dload_even(0,0,0);
		dload_odd(0,0,0);
		aload(null, null, null, null, null);
	}

	public static void iconst(int i0){
		i0=-1;
		i0=0;
		i0=1;
		i0=2;
		i0=3;
		i0=4;
		i0=5;
		i0=6;
		i0=200;
		i0=133000;
	}

	public static void xaload(int[] is, long[] ls, float[] fs, double ds[], Object as[], byte[] bs, char[] cs, short[] ss){
		is[0]=is[1];
		ls[0]=ls[1];
		fs[0]=fs[1];
		ds[0]=ds[1];
		as[0]=as[1];
		bs[0]=bs[1];
		cs[0]=cs[1];
		ss[0]=ss[1];
	}

	public static int returnInt(){ return 0; }
	public static long returnLong(){ return 0; }
	public static float returnFloat(){ return 0; }
	public static double returnDouble(){ return 0; }
	public static Object returnReference(){ return null; }
	public static void pop(){
		returnInt();
		returnLong();
		returnFloat();
		returnDouble();
		returnReference();
	}

	public static void math(){
		int i=1; long l=1; float f=1; double d=1;
		/* Doing division first prevents divide-by-zero */
		i=i/i;
		l=l/l;
		f=f/f;
		d=d/d;
		i=i%i;
		l=l%l;
		f=f%f;
		d=d%d;
		i=i+i;
		l=l+l;
		f=f+f;
		d=d+d;
		i=i-i;
		l=l-l;
		f=f-f;
		d=d-d;
		i=i*i;
		l=l*l;
		f=f*f;
		d=d*d;
		i=-i;
		l=-l;
		f=-f;
		d=-d;
		i=i<<1;
		l=l<<1;
		i=i>>1;
		l=l>>1;
		i=i&i;
		l=l&l;
		i=i|i;
		l=l|l;
		i=i^i;
		l=l^l;
		i=i++;
	}

	public static void convert(){
		int i=0; long l=0; float f=0; double d=0;
		byte b; char c; short s;
		l=i;
		f=i;
		d=i;
		i=(int)l;
		f=l;
		d=l;
		i=(int)f;
		l=(long)f;
		d=f;
		i=(int)d;
		l=(long)d;
		f=(float)d;
		b=(byte)i;
		c=(char)i;
		s=(short)i;
	}

    public static void dummy(int arg){}

	public void compare(int i0, int i1, long l, float f, double d, Object a){
		if(l < l){
			dummy(41);
		}
		if(f > f){
			dummy(1);
		}
		if(d >= d){
			dummy(2);
		}
		if(f <= d){
			dummy(3);
		}
		if(d==d){
			dummy(11);
		}
		if(f != f){
			dummy(12);
		}
		if(f <= f){
			dummy(13);
		}
		if(i0==i1){
			dummy(21);
		}
		if(i0!=i1){
			dummy(22);
		}
		if(i0<i1){
			dummy(23);
		}
		if(i0<=i1){
			dummy(24);
		}
		if(i0>i1){
			dummy(25);
		}
		if(i0>=i1){
			dummy(26);
		}
		if(this==a){
			dummy(31);
		}
		if(this!=a){
			dummy(32);
		}
	}

	public static void jsr(int arg) throws AllCodes{
		try{
			dummy(1);
			if(arg == 0){
				throw new AllCodes();
			}else{
				dummy(2);
			}
		}finally{
			dummy(21);
		}
	}

	private static int staticInt=0;
	private int nonstaticInt;
	public void fieldAccess(){
		staticInt = nonstaticInt;
		nonstaticInt = staticInt;
		try{
			AllCodes blank = null;
			staticInt = blank.nonstaticInt;
		}catch(NullPointerException e){
			dummy(1);
		}
	}

	public static void invoke(Integer i, Runnable r){
		if(i.equals(i))
			dummy(1);
		r.run();
	}

	public static Object[] news(){
		Integer i = new Integer(5);
		int[] is = new int[6];
		Object[] result = new Object[7];
		result[1] = i;
		result[3] = is;
		return result;
	}

	public static void switches(int i){
		switch(i){
		case 3:
			dummy(1);
			break;
		case 4:
			dummy(2);
			break;
		case 5:
			dummy(3);
			break;
		default:
			dummy(4);
			break;
		}
		switch(i){
		case -56:
			dummy(21);
			break;
		case 88:
			dummy(22);
			break;
		default:
			dummy(23);
			break;
		}
	}

	public static void xconst(){
		long l; float f; double d;
		l=0;
		l=1;
		l=2;
		l=3;
		f=0;
		f=1;
		f=2;
		f=3;
		d=0;
		d=1;
		d=2;
	}

	public static void misc(Object[] array){
		int i0, i1, i2, i3, i4; Object obj, obj2; Object[] os;
		obj = null; int[][][] oss;
		i0 = array.length;
		synchronized(array){
			if(array instanceof String[])
				dummy(11);
			os = (Object[]) array;
		}
		oss = new int[3][4][5];
		oss[1][2][3] = 4;
		if(oss == null)
			dummy(1);
		if(oss != null)
			dummy(2);
	}

	public static void main(String[] args){
		AllCodes instance = new AllCodes();
		instance.fieldAccess();
		try{
			jsr(0);
		} catch(AllCodes a){
		}
		misc(args);
		math();
		iconst(0);
		xconst();
		instance.compare(0,0,0,0,0,null);
		convert();
		xload();
		pop();
		invoke(new Integer(5), instance);
		news();
	}

}

