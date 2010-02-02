public class Interrupt_Thread2 extends Thread {
	public boolean sleeping;

	public Interrupt_Thread2(){
		sleeping = false;
	}

	public void run(){
		try {
			sleeping = true;
			sleep(12000);
		} catch (InterruptedException e){
			System.out.println("Interrupt_Thread interrupt");
		}
	}
}

