public class Random {
    public Random(long seed)
    {
        setSeed(seed);
    }

    public void setSeed(long seed)
    {
        this.seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
    }

    public int next(int bits)
    {
        seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
        return (int) (seed >>> (48 - bits));
    }

    public int nextInt()
    {
        return next(32);
    }

    public double nextDouble()
    {
        return (((long) next(26) << 27) + next(27)) / (double) (1L << 53);
    }

    private long seed;
}
