
class MicroBench{
	public static void routine(int i){
	}

	public static void main(String[] args){
		int i;
		for(i=0; i < 40000000; i++){
			routine(i);
		}
	}
}

