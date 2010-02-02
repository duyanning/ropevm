public class Thread3 extends Thread {
    private int iterations;

    public Thread3(int i) {
        iterations = i;
    }

    public void run() {

        for (int i = 0; i < iterations; i++) {
            yield();
        }
    }
}
