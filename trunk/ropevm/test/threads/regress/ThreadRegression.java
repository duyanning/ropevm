public class ThreadRegression {

	public static void main(String[] args) {
		boolean random = false;
		boolean simulated = false;

		if (!random) {
			/* currentThread */
			Test_CurrentThread();

			/* Name */
			Test_Name();

			/* Priority */
			Test_Priority();

			/* Interrupt */
			// Interruptions are not fully functional
			// Test_Interrupt();

			/* Alive */
			Test_Alive();

			/* Daemon */
			Test_Daemon();
		} else {
			int iterations = 10;
			String series = "4425404223";

			/* previous failed tests */
			/* String series = "4425404223"; */
			/* String series = "222205033";  */

			for (int i=0; (i<iterations && !simulated) || (i<series.length() && simulated ); i++) {
				int test;

				if (simulated) {
					test = series.charAt(i) - '0';
				} else {
					test = (int)(Math.random()*6);
				}

				System.out.println("*************");
				System.out.println("* Test #: "+test+" *");
				System.out.println("*************");

				switch(test) {

				case 0:
					/* currentThread */
					Test_CurrentThread();
					break;

				case 1:
					/* Name */
					Test_Name();
					break;

				case 2:
					/* Priority */
					Test_Priority();
					break;

				case 3:
					/* Interrupt */
					// Interruptions are not fully functional
					// Test_Interrupt();
					break;

				case 4:
					/* Alive */
					Test_Alive();
					break;

				case 5:
					/* Daemon */
					Test_Daemon();
					break;
				}
			}
		}
	}

	private static void Test_CurrentThread() {
		System.out.println("******************");
		System.out.println("* Current Thread *");
		System.out.println("******************");

		Current current = new Current();
		current.start();

		try {
			current.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		System.out.println("Main currentThread: "+Thread.currentThread());
	}

	private static void Test_Name() {
		System.out.println("*************");
		System.out.println("* Test Name *");
		System.out.println("*************");

		Name name1 = new Name("thread1");
		Name name2 = new Name("thread2");

		name1.start();

		try {
			name1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		name2.start();

		try {
			name2.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		System.out.println("Main getName: "+Thread.currentThread().getName());
	}

	private static void Test_Priority() {
		System.out.println("************");
		System.out.println("* Priority *");
		System.out.println("************");

		Priority priority = new Priority();
		priority.start();

		try {
			priority.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		System.out.println("Main Priority: "+Thread.currentThread().getPriority());
	}

	private static void Test_Interrupt() {
		System.out.println("*************");
		System.out.println("* Interrupt *");
		System.out.println("*************");

		Interrupt interrupt = new Interrupt();
		interrupt.start();

		try {
			interrupt.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}

	private static void Test_Alive() {
		System.out.println("*********");
		System.out.println("* Alive *");
		System.out.println("*********");

		Alive alive = new Alive();
		alive.start();

		try {
			alive.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}

	private static void Test_Daemon() {
		System.out.println("**********");
		System.out.println("* Daemon *");
		System.out.println("**********");

		Daemon daemon = new Daemon();
		daemon.start();

		try {
			daemon.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}
	}
}

