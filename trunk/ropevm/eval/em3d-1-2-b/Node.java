
import java.util.Enumeration;
//import java.util.Random;

/**
 * This class implements nodes (both E- and H-nodes) of the EM graph. Sets
 * up random neighbors and propagates field values among neighbors.
 */
@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
final class Node {
    /**
     * The value of the node. 节点的值
     **/
    double value;

    /**
     * Array of nodes to which we send our value. “去往”节点数组
     **/
    Node[] toNodes;

    /**
     * Array of nodes from which we receive values. “来自”节点数组
     **/
    Node[] fromNodes;

    /**
     * Coefficients on the fromNodes edges 对应“来自”节点数组的系数数组
     **/
    double[] coeffs;

    /**
     * The number of fromNodes edges 节点的入度
     **/
    int fromCount;

    /**
     * Used to create the fromEdges - keeps track of the number of edges that have
     * been added
     **/
    int fromLength;

    /**
     * A random number generator.
     **/
    private static Random rand;
    //private Random rand;

    /**
     * Initialize the random number generator
     **/
    public static void initSeed(long seed)
    {
        rand = new Random(seed);
    }


    /**
     * Constructor for a node with given `degree'.   The value of the
     * node is initialized to a random value.
     * param degree 节点的出度
     **/
    Node(int degree)
    {
        //rand = new Random(783);

        value = rand.nextDouble();
        // create empty array for holding toNodes
        toNodes = new Node[degree];

        fromNodes = null;
        coeffs = null;
        fromCount = 0;
        fromLength = 0;
    }
    void __rvp__ctor(int degree)
    {
    }

    /**
     * Create unique `degree' neighbors from the nodes given in nodeTable.
     * We do this by selecting a random node from the give nodeTable to
     * be neighbor. If this neighbor has been previously selected, then
     * a different random neighbor is chosen.
     * @param nodeTable the list of nodes to choose from. 从其中为当前节点挑选若干“去往”节点
     **/
    // 本方法结束后，节点的“去往”节点就被确定下来了，节点的入度也确定下来了。
    void makeUniqueNeighbors(Node[] nodeTable)
    {
        //assert nodeTable.length <= 10;
        // 给本节点的“去往”节点数组的各个slot填充“去往”节点
        for (int filled = 0; filled < toNodes.length; filled++) {
            int k;
            Node otherNode;     // 找到的“去往”节点

            // 从另一个场中的节点（nodeTable）中找一个作为本节点的“去往”节点
            do {
                // 先随机找一个作为“去往”节点

                // generate a random number in the correct range
                int index = rand.nextInt();
                if (index < 0) index = -index;
                index = index % nodeTable.length;

                // find a node with the random index in the given table
                otherNode = nodeTable[index];

                for (k = 0; k < filled; k++) {
                    // [0, filled)是“去往”节点数组中已经填充好的部分
                    //if (otherNode == toNodes[filled]) break;
                    if (otherNode == toNodes[k]) break; // ==意味着跟某个已有节点是同一个。（原来的程序这行有bug）
                }
            } while (k < filled); // k < filled意味着找到的这个“去往”节点跟“去往”节点数组中的某个已有节点是同一个，不行，得重新找

            // other node is definitely unique among "filled" toNodes
            // 把找到的节点（即otherNode）填入当前节点（即this）的“去往”节点数组中的某个slot
            toNodes[filled] = otherNode;

            // update fromCount for the other node
            // “去往”节点的入度++
            otherNode.fromCount++;
        }
    }
    void __rvp__makeUniqueNeighbors(Node[] nodeTable)
    {
    }

    /**
     * Allocate the right number of FromNodes for this node. This
     * step can only happen once we know the right number of from nodes
     * to allocate. Can be done after unique neighbors are created and known.
     *
     * It also initializes random coefficients on the edges.
     **/
    // 按照节点的入度创建“来自”节点数组，以及相应的系数数组
    void makeFromNodes()
    {
        fromNodes = new Node[fromCount]; // nodes fill be filled in later
        coeffs = new double[fromCount];
    }
    void __rvp__makeFromNodes()
    {
    }

    /**
     * Fill in the fromNode field in "other" nodes which are pointed to
     * by this node.
     **/
    // 填充本节点的“来自”节点数组，以及相应的系数数组
    void updateFromNodes()
    {

        for (int i = 0; i < toNodes.length; i++) {
            Node otherNode = toNodes[i];
            int count = otherNode.fromLength++;
            otherNode.fromNodes[count] = this;
            otherNode.coeffs[count] = rand.nextDouble();
        }
    }
    void __rvp__updateFromNodes()
    {
    }

    /**
     * Get the new value of the current node based on its neighboring
     * from_nodes and coefficients.
     **/
    // 根据各个“来自”节点的值及相应的系数，更新本节点的值
    void computeNewValue()
    {
        for (int i = 0; i < fromCount; i++) {
            value -= coeffs[i] * fromNodes[i].value;
        }

        // for (int j = 0; j < 5000; j++) { // 这段代码是想增加get出错的代价，以证明推测路障的好处。但是却没能证明。
        //     double x = 2.5;
        //     double y = 1.5;
        //     double z = 0.0;
        //     z = x / y;
        // }
    }
    // 快速结束，以便对下一个节点发起computeNewValue调用
    void __rvp__computeNewValue()
    {
    }


    /**
     * Override the toString method to return the value of the node.
     * @return the value of the node.
     **/
    // 打印节点的值以及入度
    public String toString()
    {
        return "value " + value + ", from_count " + fromCount;
    }

}
