
class CloneTest implements Cloneable{

	int x;

	private static void instance(){
		CloneTest c1, c2;
		c1 = new CloneTest();
		c1.x = 5;
		try{
			c2 = (CloneTest)c1.clone();
			System.out.println(c1.x);
			System.out.println(c2.x);
			c2.x = 6;
			System.out.println(c1.x);
			System.out.println(c2.x);
		}catch(CloneNotSupportedException e){
			System.out.println(e);
		}
	}

	private static void array(){
		int[] i1, i2; int idx;
		i1 = new int[5];
		for(idx=0; idx < i1.length; idx++){
			i1[idx] = idx;
		}
		i2 = (int[]) i1.clone();
		for(idx=0; idx < i1.length; idx++){
			System.out.println(i1[idx]);
			System.out.println(i2[idx]);
		}
		for(idx=0; idx < i1.length; idx++){
			i1[idx] = -idx;
		}
		for(idx=0; idx < i1.length; idx++){
			System.out.println(i1[idx]);
			System.out.println(i2[idx]);
		}
	}

	public static void main(String[] args){
		instance();
		array();
	}

}

