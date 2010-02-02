public class Interrupt extends Thread {

	public Interrupt() {
	}

	public void run() {

		/* Test interruptions on non-interruptable operations */
		Interrupt_Thread1 thread1 = new Interrupt_Thread1();

		/* before even starting the thread */
		thread1.interrupt();

		if (thread1.isInterrupted()){
			System.out.println("Interrupt_Thread1 Interrupted.");
		} else {
			System.out.println("Interrupt_Thread1 not Interrupted.");
		}

		/* start the thread */
		thread1.start();

		/* wait until it becomes alive */
		while (!thread1.counting);

		try {
			sleep(1000);
		} catch (InterruptedException e){
			System.out.println(e);
		}

		thread1.interrupt();

		if (thread1.isInterrupted()){
			System.out.println("Interrupt_Thread1 Interrupted.");
		} else {
			System.out.println("Interrupt_Thread1 not Interrupted.");
		}

		try {
			thread1.join();
			System.out.println("Interrupt_Thread1 joined");
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/************************************************************************/

		Interrupt_Thread2 thread2 = new Interrupt_Thread2();
		Interrupt_Thread3 thread3 = new Interrupt_Thread3(thread2);

		thread2.start();

		while (!thread2.isAlive());

		thread3.start();

		while (!thread2.sleeping);

		try {
			sleep(6000);
		} catch (InterruptedException e){
			System.out.println("Interrupt: "+e);
		}

		thread2.interrupt();

/*
		if (thread2.isInterrupted()){
			System.out.println("Interrupt_Thread2 Interrupted.");
		} else {
			System.out.println("Interrupt_Thread2 not Interrupted.");
		}
*/

		try {
			thread2.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		if (thread2.isInterrupted()){
			System.out.println("Interrupt_Thread2 Interrupted.");
		} else {
			System.out.println("Interrupt_Thread2 not Interrupted.");
		}

		try {
			thread3.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}
}

