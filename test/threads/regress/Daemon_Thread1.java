public class Daemon_Thread1 extends Thread {

	public Daemon_Thread1(){
	}

	public void run(){
		try {
			sleep(10000);
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}
}

