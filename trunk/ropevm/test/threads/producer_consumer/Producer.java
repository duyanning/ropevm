public class Producer extends Thread {
	private CubbyHole cubbyhole;
	private int number;
	private int max_iterations;
	private int[] put_array;

	public Producer(CubbyHole c, int number, int max_iterations, int[] put_array) {
		cubbyhole = c;
		this.number = number;
		this.max_iterations = max_iterations;
		this.put_array = put_array;
	}

	public void run() {
		for (int i = 0; i < max_iterations; i++) {
			cubbyhole.put(i);
			synchronized (put_array) {
				put_array[i]++;
			}
//			System.out.println("Producer #" + this.number
//			System.out.println("Producer "
//				   + " put: " + i);
			try {
				sleep((int)(Math.random() * 100));
			} catch (InterruptedException e) { }
		}
	}
}
