public class Daemon_Thread2 extends Thread {
	private Daemon_Thread1 thr;

	public Daemon_Thread2(Daemon_Thread1 thr1){
		thr = thr1;
	}

	public void run(){
		while(thr.isAlive());
		System.out.println("Thread daemon alive");
	}
}

