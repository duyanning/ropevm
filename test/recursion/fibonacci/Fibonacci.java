public class Fibonacci {
    int n;

    public static void main(String args[]) {
        if (args.length > 0)
            System.out.println(new Fibonacci(Integer.parseInt(args[0])).get());
    }

    public Fibonacci(int n) {
        this.n = n;
    }

    public int get() {
        if (n == 1 || n == 2)
            // simple case
            return 1;
        else
            // recursive case
            return new Fibonacci(n - 1).get() + new Fibonacci(n - 2).get();
    }
}
