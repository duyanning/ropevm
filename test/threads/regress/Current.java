public class Current extends Thread {

	public Current() {
	}

	public void run() {
		Thread thr = Thread.currentThread();

		System.out.println("Thread currentThread: "+thr);
		System.out.println("Thread currentThread: Thread.currentThread()==this?"+(thr==this));
	}
}

