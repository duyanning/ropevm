public class Factorial {
    public static int factorialIterative(int n) {
        // Iterative factorial function
        int solution = 1;
        for (int i = 1; i <= n; i++)
            solution *= i;
        return solution;
    }

    public static int factorialRecursive(int n) {
        // Recursive factorial function
        if (n == 1)
            return 1;
        else
            return n * factorialRecursive(n - 1);
    }

}
    