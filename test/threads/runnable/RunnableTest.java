import java.lang.*;

public class RunnableTest {

	public static void main(String argv[]) {
		int NUM_THREADS = new Integer(argv[0]).intValue();
		int LENGTH = new Integer(argv[1]).intValue();
		TestObject o = new TestObject(NUM_THREADS,LENGTH);
		o.run();
	}
}

class TestObject {

	byte[][] text;
	int nthreads;
	int length;

	public TestObject(int nthreads, int length) {
		this.nthreads = nthreads;
		this.length = length;
	}

	public void run() {

		Runnable thobjects[] = new Runnable[nthreads];
		Thread th[] = new Thread[nthreads];

		text = new byte[nthreads][length];

		th = new Thread[nthreads];

		for (int i=1; i<nthreads; i++) {
			thobjects[i] = new Runner(text[i]);
			th[i] = new Thread(thobjects[i]);
			th[i].start();
		}

		thobjects[0] = new Runner(text[0]);
		thobjects[0].run();

		for (int i=1; i<nthreads; i++) {
			try {
				th[i].join();
			}
			catch (InterruptedException e) {}
		}

		boolean OK = true;
		for (int i=0; i<nthreads && OK; i++) {
			for (int j=0; j<length && OK; j++) {
				int i2 = (i+1)%nthreads;
				if (text[i][j] != text[i2][j]) {
					OK = false;
					System.out.println("Thread 1: "+i+" "+text[i][j]);
					System.out.println("Thread 2: "+i2+" "+text[i2][j]);
				} else {
					System.out.print(text[i][j]);
				}
			}
		}

		System.out.println("");
		if (OK) System.out.println ("Results OK");
		else System.out.println("Results WRONG");
	}
}

class Runner implements Runnable {

	byte text[];

	public Runner(byte[] text) {
		this.text = text;
	}

	public void run() {

		for (int i=0; i<text.length; i++) {
			text[i] = (byte) i;
		}

		byte x1;

		for (int i=0; i<text.length; i++) {
			x1 = (byte) (text[i] & 0xff);
			x1 |= (text[i] & 0xff) << 8;
			x1 |= i+1;
			x1 |= i+2;
			text[i] = x1;
		}
	}
}

