public class ProducerConsumerTest {
	public static void main(String[] args) {
		CubbyHole ch = new CubbyHole();

		int MAX_ITERATIONS = new Integer(args[0]).intValue();
		int put_array[] = new int[MAX_ITERATIONS];
		int get_array[] = new int[MAX_ITERATIONS];

		int NUM_PRODUCERS = new Integer(args[1]).intValue();
		int NUM_CONSUMERS = new Integer(args[2]).intValue();

		Producer p[] = new Producer[NUM_PRODUCERS];
		Consumer c[] = new Consumer[NUM_CONSUMERS];

		System.out.println("Starting consumer threads ...");
		for (int i=0; i<NUM_CONSUMERS-1; i++) {
			if (i==0 && (NUM_PRODUCERS % NUM_CONSUMERS != 0)) {
				c[i] = new Consumer(ch, i, (NUM_PRODUCERS / NUM_CONSUMERS) * MAX_ITERATIONS + (NUM_PRODUCERS % NUM_CONSUMERS) * MAX_ITERATIONS, get_array);
			} else {
				c[i] = new Consumer(ch, i, (NUM_PRODUCERS / NUM_CONSUMERS) * MAX_ITERATIONS, get_array);
			}
			c[i].start();
		}

		System.out.println("Starting producer threads ...");
		for (int i=0; i<NUM_PRODUCERS; i++) {
			p[i] = new Producer(ch, i, MAX_ITERATIONS, put_array);
			p[i].start();
		}

		Consumer consumer;
		if (NUM_CONSUMERS == 1 && (NUM_PRODUCERS % NUM_CONSUMERS != 0)) {
			consumer = new Consumer(ch, 0, (NUM_PRODUCERS / NUM_CONSUMERS) * MAX_ITERATIONS + (NUM_PRODUCERS % NUM_CONSUMERS) * MAX_ITERATIONS, get_array);
		} else {
			consumer = new Consumer(ch, NUM_CONSUMERS-1, (NUM_PRODUCERS / NUM_CONSUMERS) * MAX_ITERATIONS, get_array);
		}
		consumer.run();

		for (int i=0; i<NUM_PRODUCERS; i++) {
			try {
				p[i].join();
			} catch (InterruptedException e) {
				i--;
			}
		}

		for (int i=0; i<NUM_CONSUMERS-1; i++) {
			try {
				c[i].join();
			} catch (InterruptedException e) {
				i--;
			}
		}

		/* Test whether each value was produced and consumed the same
		   number of times. */
		boolean OK = true;
		for (int i=0; i<MAX_ITERATIONS && OK; i++) {
			if (put_array[i] != get_array[i]) {
				OK = false;
				System.out.println("Number: "+i);
				System.out.println("Put: "+put_array[i]);
				System.out.println("Get: "+get_array[i]);
			}
		}

		if (OK) System.out.println("Results OK");
		else System.out.println("Results WRONG");
	}
}

