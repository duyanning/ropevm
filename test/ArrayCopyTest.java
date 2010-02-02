
class ArrayCopyTest{

	private static void copy(int src, int dest, int len){
		byte bytes[] = {0, 1, 2, 3, 4, 5};
		short shorts[] = {1000, 1101, 1202, 1303, 1404, 1505};
		int ints[] = {1000000, 1100001, 1200002, 1300003, 1400004, 1500005};
		long longs[] = {1000000000000L, 1100000000001L, 1200000000002L,
			1300000000003L, 1400000000004L, 1500000000005L};
		System.out.println("copy(" + src + ", " + dest + ", " + len + ")");
		try{
			System.arraycopy(bytes,  src, bytes,  dest, len);
			for(int idx=0; idx < bytes.length; idx++)
				System.out.println(bytes[idx]);
		}catch(IndexOutOfBoundsException e){
			System.out.println("Bad byte");
		}
		try{
			System.arraycopy(shorts, src, shorts, dest, len);
			for(int idx=0; idx < shorts.length; idx++)
				System.out.println(shorts[idx]);
		}catch(IndexOutOfBoundsException e){
			System.out.println("Bad short");
		}
		try{
			System.arraycopy(ints, src, ints, dest, len);
			for(int idx=0; idx < ints.length; idx++)
				System.out.println(ints[idx]);
		}catch(IndexOutOfBoundsException e){
			System.out.println("Bad int");
		}
		try{
			System.arraycopy(longs, src, longs, dest, len);
			for(int idx=0; idx < longs.length; idx++)
				System.out.println(longs[idx]);
		}catch(IndexOutOfBoundsException e){
			System.out.println("Bad long");
		}
	}

	private static void mismatches(){
		int ints[] = new int[2];
		short shorts[] = new short[2];
		Object obs[] = {"str1", null, System.out};
		String obs2[] = {null, "str2", "str3"};
		try{
			System.arraycopy(ints, 0, shorts, 0, 1);
			System.out.println("NO EXCEPTION");
		}catch(ArrayStoreException e){
			System.out.println("ArrayStoreException");
		}catch(Throwable e){
			System.out.println("WRONG EXCEPTION: " + e.toString());
		}
		try{
			System.arraycopy(obs, 0, ints, 0, 1);
			System.out.println("NO EXCEPTION");
		}catch(ArrayStoreException e){
			System.out.println("ArrayStoreException");
		}catch(Throwable e){
			System.out.println("WRONG EXCEPTION: " + e.toString());
		}
		try{
			System.arraycopy(shorts, 0, obs, 0, 1);
			System.out.println("NO EXCEPTION");
		}catch(ArrayStoreException e){
			System.out.println("ArrayStoreException");
		}catch(Throwable e){
			System.out.println("WRONG EXCEPTION: " + e.toString());
		}
		System.out.println(obs2[0]);
		System.out.println(obs2[1]);
		System.out.println(obs2[2]);
		try{
			System.arraycopy(obs, 0, obs2, 0, 3);
			System.out.println("NO EXCEPTION");
		}catch(ArrayStoreException e){
			System.out.println("ArrayStoreException");
		}catch(Throwable e){
			System.out.println("WRONG EXCEPTION: " + e.toString());
		}
		System.out.println(obs2[0]);
		System.out.println(obs2[1]);
		System.out.println(obs2[2]);
	}

	public static void main(String[] args){
		copy(1, 2, -1);
		copy(1, 2, 0);
		copy(1, 2, 1);
		copy(1, 2, 3);
		copy(2, 1, -1);
		copy(2, 1, 0);
		copy(2, 1, 1);
		copy(2, 1, 3);
		copy(1, -1, 1);
		copy(1, 6, 1);
		copy(-1, 1, 1);
		copy(6, 1, 1);
		copy(0, 5, 1);
		copy(5, 0, 1);
		mismatches();
	}

}

