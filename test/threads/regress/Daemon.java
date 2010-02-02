public class Daemon extends Thread {

	public Daemon() {
	}

	public void run() {
		Daemon_Thread1 thread1 = new Daemon_Thread1();
		Daemon_Thread2 thread2 = new Daemon_Thread2(thread1);
		boolean daemon = true;

		thread1.setDaemon(daemon);
		thread1.start();

		if(daemon==thread1.isDaemon()){
			System.out.println("Daemon_Thread is daemon");
		}

		thread2.start();

		try {
			thread1.setDaemon(!daemon);
		} catch (IllegalThreadStateException e){
			System.out.println("IllegalThreadStateException caught");
		}

		try {
			thread2.join();
		} catch (InterruptedException e){
			System.out.println("Thead ERROR");
		}

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println("Thead ERROR");
		}
	}
}

