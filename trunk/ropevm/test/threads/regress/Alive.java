public class Alive extends Thread {

	public Alive() {
	}

	public void run() {
		Alive_Thread1 thread1 = new Alive_Thread1();
		Alive_Thread2 thread2 = new Alive_Thread2(thread1);

		thread1.start();

		while (!thread1.isAlive());

		thread2.start();

		try {
			thread2.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}
}

