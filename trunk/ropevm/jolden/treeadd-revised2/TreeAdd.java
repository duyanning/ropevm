
import java.io.*;

/**
 * A Java implementation of the <tt>treeadd</tt> Olden benchmark.
 * <p>
 * Treeadd is a very simple program that performs a recursive depth
 * first traversal of a binary tree and sums the value of each element
 * in the tree.  We initialize the elements in the tree to contain
 * '1'.
 **/
public class TreeAdd
{
    /**
     * The number of levels in the tree.
     **/
    private static int levels = 0;
    /**
     * Set to true to print the final result.
     **/
    private static boolean printResult = false;
    /**
     * Set to true to print informative messages
     **/
    private static boolean printMsgs = false;

    /**
     * The main routine which creates a tree and traverses it.
     * @param args the arguments to the program
     **/
    public static void main(String args[])
    {
        parseCmdLine(args);
        preloadClasses();

        Tree tree1 = new Tree(levels);
        Tree tree2 = new Tree(levels);

        int result1 = tree1.addTree();
        int result2 = tree2.addTree();

        if (printResult || printMsgs)
            System.out.println("Received results of tree1 " + result1);
        if (printResult || printMsgs)
            System.out.println("Received results of tree2 " + result2);

        System.out.println("Done!");
    }

    static void preloadClasses()
    {
        new Tree();
        new TreeNode();
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

            if (arg.equals("-l")) {
                if (i < args.length) {
                    levels = new Integer(args[i++]).intValue();
                } else throw new RuntimeException("-l requires the number of levels");
            } else if (arg.equals("-p")) {
                printResult = true;
            } else if (arg.equals("-m")) {
                printMsgs = true;
            } else if (arg.equals("-h")) {
                usage();
            }
        }
        if (levels == 0) usage();
    }

    /**
     * The usage routine which describes the program options.
     **/
    private static final void usage()
    {
        System.err.println("usage: java TreeAdd -l <levels> [-p] [-m] [-h]");
        System.err.println("    -l the number of levels in the tree");
        System.err.println("    -m (print informative messages)");
        System.err.println("    -p (print the result>)");
        System.err.println("    -h (this message)");
        System.exit(0);
    }

}
