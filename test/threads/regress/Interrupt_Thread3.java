public class Interrupt_Thread3 extends Thread {
	private Interrupt_Thread2 thr;

	public Interrupt_Thread3(Interrupt_Thread2 thr2){
		thr = thr2;
	}

	public void run(){
		boolean interrupted=false;

		if (thr.isAlive()){
			System.out.println("Interrupt_Thread 2: Alive.");
		} else {
			System.out.println("Interrupt_Thread 2: not Alive.");
		}

		while(thr.isAlive() && !interrupted){
				yield();
				interrupted=thr.isInterrupted();
		}
		if (interrupted){
			System.out.println(thr+" has been interrupted.");
		}
	}
}

