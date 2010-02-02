public class Interrupt_Thread1 extends Thread {
	public boolean counting;

	public Interrupt_Thread1(){
		counting = false;
	}

	public void run() {
		counting = true;
		/* Perform some useless computations */
		for (int i=0; i<10000; i++);
	}
}

