
class NativeBench extends Benchmark{
	protected void action(int iterCount){
		Util.emptyLoop(iterCount);
	}

	public static void main(String args[]){
		new NativeBench().run(args);
	}
}

