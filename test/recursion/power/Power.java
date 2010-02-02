public class Power {

    public static int powerRecursive(int n, int exp) {
        // Recursive power function
        if (exp == 0)
            return 1;
        else
            return n * powerRecursive(n, exp - 1);
    }

}
    
