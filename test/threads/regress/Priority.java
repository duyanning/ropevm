public class Priority extends Thread {

	public Priority() {
	}

	public void run() {
		Priority_Thread1 thread1;

		/*********************************************************************/
		/* getPriority */
		System.out.println("***************");
		System.out.println("* getPriority *");
		System.out.println("***************");

		thread1 = new Priority_Thread1();
		thread1.start();
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/*********************************************************************/
		/* getPriority + setPriority */
		System.out.println("*****************************");
		System.out.println("* getPriority + setPriority *");
		System.out.println("*****************************");

		/* MIN_PRIORITY */
		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.MIN_PRIORITY);
		thread1.start();
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/* NORM_PRIORITY */
		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.NORM_PRIORITY);
		thread1.start();
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/* MAX_PRIORITY */
		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.MAX_PRIORITY);
		thread1.start();
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/*********************************************************************/
		/* setPriority stress */
		System.out.println("**********************");
		System.out.println("* setPriority stress *");
		System.out.println("**********************");

		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.MIN_PRIORITY);
		thread1.start();

		for (int i=0;i<10;i++){
			thread1.setPriority(thread1.NORM_PRIORITY);
			thread1.setPriority(thread1.MAX_PRIORITY);
			thread1.setPriority(thread1.MIN_PRIORITY);
		}

		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		/*********************************************************************/
		/* setPriority after the process died */
		System.out.println("**************************");
		System.out.println("* setPriority after dead *");
		System.out.println("**************************");

		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.MAX_PRIORITY);
		thread1.start();
		thread1.print();

		while(thread1.isAlive());

		thread1.setPriority(thread1.NORM_PRIORITY);
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		thread1.setPriority(thread1.MIN_PRIORITY);
		thread1.print();

		/*********************************************************************/
		/* setPriority after join */
		System.out.println("**************************");
		System.out.println("* setPriority after join *");
		System.out.println("**************************");

		thread1 = new Priority_Thread1();
		thread1.setPriority(thread1.MAX_PRIORITY);
		thread1.start();
		thread1.print();

		try {
			thread1.join();
		} catch (InterruptedException e){
			System.out.println(e);
		}

		thread1.setPriority(thread1.MIN_PRIORITY);
		thread1.print();

		thread1.setPriority(thread1.NORM_PRIORITY);
		thread1.print();
	}
}

