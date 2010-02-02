public class Name extends Thread {

	private String name;

	public Name(String str) {
		name = str;
	}

	public void run() {
		Thread thr = Thread.currentThread();

		System.out.println("Thread getName: "+thr.getName());

		thr.setName(name);
		System.out.println("Thread setName: "+thr.getName());
	}
}

