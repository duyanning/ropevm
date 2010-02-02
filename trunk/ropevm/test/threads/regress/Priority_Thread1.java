public class Priority_Thread1 extends Thread {
	int var;

	public Priority_Thread1(){
	}

	private void dummy(int i) {
		var = i * 3 + i;
	}

	public void run() {
		/* Some useless computations */
		for (int i=0; i<10; i++) {
			dummy(i);
			try {
				sleep(1000);
			} catch (InterruptedException e){
				System.out.println(e);
			}
		}
	}

	public void print() {
		System.out.println("Priority_Thread getPriority: "+getPriority());
	}
}

