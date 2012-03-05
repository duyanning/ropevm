
import java.io.*;

/**
 * A Java implementation of the <tt>power</tt> Olden benchmark.
 * The original algorithm is from the following paper:
 * <p><cite>
 * S. Lumetta, L. Murphy, X. Li, D. Culler, and I. Khalil.
 * "Decentralized optimal power pricing: The development of a parallel
 * program."  Supercomputing '93, 243-249
 * </cite>
 * <p>
 * Note - the number of customers is fixed at 10,000 for this
 * benchmark.  We create a data structure that contains 1 root (the
 * (power substation).  There are 10 main feeders from the root and
 * each feeder branches to 20 lateral nodes.  Each lateral node is the
 * head of a line of five branch nodes and each branch has 10
 * customers.  In total, there are 10,000 customers (and 1201 internal
 * nodes).
 * <p>
 * The power pricing problems sets the price of each customer's power
 * consumption so that the economic efficiency of the whole community
 * is maximized.
 **/
public class Power
{
    /**
     * Should we print the results as we run the benchmark
     **/
    static boolean printResults = false;
    /**
     * Print information messages?
     **/
    static boolean printMsgs = false;

    /**
     * The main routine which creates the power network and runs the simulation
     * @param args the command line args
     **/
    public static void main(String args[])
    {
        preloadClasses();
        // the input size is fixed, but the user may want the result printed
        parseCmdLine(args);

        // initial pass
        RopeVMBackdoor.turn_on_probe();
        long start0 = 0;
        if (printMsgs)
            start0 = System.currentTimeMillis();

        //Root r = new Root(10, 20, 5, 10);
        Root r = new Root(4, 2, 2, 2);
        long end0 = 0;
        if (printMsgs)
            end0 = System.currentTimeMillis();
        // RopeVMBackdoor.turn_off_probe();

        // ================================================================

        // 以上的构造部分，加速比有1.9181。本来要构造2组共4个lateral，因为循环，两组并行构造。每组中，因为递归，也可以并行构造。加速比应当接近4。

        //RopeVMBackdoor.turn_on_probe();
        long start1 = 0;
        if (printMsgs)
            start1 = System.currentTimeMillis();

        r.compute();
        r.nextIter(false, 0.7, 0.14);

        // 目前只迭代一次，看看加速比的变化。
        while (true) {
            r.compute();
            if (printResults)
                System.out.println(r);

            if (r.reachedLimit())
                break;

            r.nextIter(printResults);
        } /* while */

        long end1 = 0;
        if (printMsgs)
            end1 = System.currentTimeMillis();
        RopeVMBackdoor.turn_off_probe();


        if (printMsgs) {
            System.out.println("Power build time " + (end0 - start0)/1000.0);
            System.out.println("Power compute time " + (end1 - start1)/1000.0);
            System.out.println("Power total time " + (end1 - start0)/1000.0);
        }
        System.out.println("Done!");
    }

    static void preloadClasses()
    {
        MiniClassPathHelper.preloadClasses();
        try {
            Class.forName("Root");
            Class.forName("Lateral");
            Class.forName("Branch");
            Class.forName("Leaf");
            Class.forName("Demand");
        }
        catch (ClassNotFoundException e) {
            System.out.println("forName cannot find: " + e);
        }
    }

    /**
     * Parse the command line options.
     * @param args the command line options.
     **/
    private static final void parseCmdLine(String args[])
    {
        int i = 0;
        String arg;

        while (i < args.length && args[i].startsWith("-")) {
            arg = args[i++];
            if (arg.equals("-h")) {
                usage();
            } else if (arg.equals("-p")) {
                printResults = true;
            } else if (arg.equals("-m")) {
                printMsgs = true;
            }
        }
    }

    /**
     * The usage routine which describes the program options.
     **/
    private static final void usage()
    {
        System.err.println("usage: java Power [-p] [-m] [-h]");
        System.err.println("    -p (print results)");
        System.err.println("    -m (print informative messages)");
        System.err.println("    -h (this message)");
        System.exit(0);
    }

}
