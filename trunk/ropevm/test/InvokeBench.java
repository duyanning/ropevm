
class InvokeBench extends Benchmark{

	int counter;

	protected void action(int iterCount){
		counter = iterCount;
		while(counter > 0)
			decrement();
	}

	private void decrement(){
		counter--/*
			Had to make this access a field, or else JDK optimizes
			the whole method call away.
		*/;
	}

	public static void main(String[] args){
		new InvokeBench().run(args);
	}

}

