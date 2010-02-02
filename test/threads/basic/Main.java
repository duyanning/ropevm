public class Main {
    public static void main(String[] args) {

        Thread1 t1 = new Thread1(new Integer(args[0]).intValue());
        Thread2 t2 = new Thread2(new Integer(args[0]).intValue());

        t1.start();
        t2.start();

	try {
		t1.join();
		t2.join();
	} catch (InterruptedException e) {
		System.out.println(e);
	}

	System.out.println("[MAIN] End main");
    }
}
