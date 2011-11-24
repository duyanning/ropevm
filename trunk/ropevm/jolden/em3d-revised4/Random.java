// 等同于库中的同名类，单独拿出来是为了避免修改库代码
public class Random {

    @RopeConst
    public Random(long seed)
    {
        setSeed(seed);
    }

    @RopeConst
    public void setSeed(long seed)
    {
        this.seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
    }

    @RopeConst
    public int next(int bits)
    {
        seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
        return (int) (seed >>> (48 - bits));
    }

    @RopeConst
    public int nextInt()
    {
        return next(32);
    }

    @RopeConst
    public double nextDouble()
    {
        return (((long) next(26) << 27) + next(27)) / (double) (1L << 53);
    }

    private long seed;
}
