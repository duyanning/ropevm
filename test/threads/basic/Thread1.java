public class Thread1 extends Thread {
    private int iterations;

    public Thread1(int i) {
	iterations = i;
    }

    public void run() {

	Thread3 t3 = new Thread3(iterations);

	t3.start();

        for (int i = 0; i < iterations; i++) {
            yield();
        }

	try {
		t3.join();
	} catch (InterruptedException e) {
		System.out.println(e);
	};
    }
}
