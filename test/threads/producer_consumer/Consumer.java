public class Consumer extends Thread {
	private CubbyHole cubbyhole;
	private int number;
	private int max_iterations;
	private int[] get_array;

	public Consumer(CubbyHole c, int number, int max_iterations, int[] get_array) {
		cubbyhole = c;
		this.number = number;
		this.max_iterations = max_iterations;
		this.get_array = get_array;
	}

	public void run() {
		int value = -1;
		for (int i = 0; i < max_iterations; i++) {
			value = cubbyhole.get();
			synchronized(get_array) {
				get_array[value]++;
			}

//		System.out.println("Consumer #" + this.number
//		System.out.println("Consumer "
//			+ " got: " + value);
		}
	}
}

