public class Alive_Thread2 extends Thread {
	private Alive_Thread1 thr;

	public Alive_Thread2(Alive_Thread1 thr1){
		thr = thr1;
	}

	public void run(){
		while(thr.isAlive());
		System.out.println("Thread alive");
	}
}

