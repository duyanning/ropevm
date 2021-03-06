
import java.util.Enumeration;

/**
 * A class that represents the irregular bipartite graph used in
 * EM3D.  The graph contains two linked structures that represent the
 * E nodes and the N nodes in the application.
 **/
final class BiGraph
{
    /**
     * Nodes that represent the electrical field.
     **/
    Node eTable[];
    /**
     * Nodes that representhe the magnetic field.
     **/
    Node hTable[];


    /**
     * Create the bi graph that contains the linked list of
     * e and h nodes.
     * @param numNodes the number of nodes to create 每个场中有多少个节点
     * @param numDegree the out-degree of each node 每个节点的出度
     * @param verbose should we print out runtime messages
     **/
    BiGraph(int numNodes, int numDegree, boolean verbose)
    {
        Node.initSeed(783);

        // making nodes (we create a table)
        if (verbose) System.out.println("making nodes (tables in orig. version)");

        hTable = new Node[numNodes];
        for (int i = 0; i < hTable.length; i++) {
            hTable[i] = new Node(numDegree);
        }

        eTable = new Node[numNodes];
        for (int i = 0; i < eTable.length; i++) {
            eTable[i] = new Node(numDegree);
        }


        // making neighbors
        if (verbose) System.out.println("updating from and coeffs");

        for (int i = 0; i < hTable.length; i++) {
            Node n = hTable[i];
            n.makeUniqueNeighbors(eTable);
        }

        for (int i = 0; i < eTable.length; i++) {
            Node n = eTable[i];
            n.makeUniqueNeighbors(hTable);
        }


        // Create the fromNodes and coeff field
        if (verbose) System.out.println("filling from fields");

        for (int i = 0; i < hTable.length; i++) {
            Node n = hTable[i];
            n.makeFromNodes();
        }

        for (int i = 0; i < eTable.length; i++) {
            Node n = eTable[i];
            n.makeFromNodes();
        }


        // Update the fromNodes
        for (int i = 0; i < hTable.length; i++) {
            Node n = hTable[i];
            n.updateFromNodes();
        }

        for (int i = 0; i < eTable.length; i++) {
            Node n = eTable[i];
            n.updateFromNodes();
        }

    }

    /**
     * Update the field values of e-nodes based on the values of
     * neighboring h-nodes and vice-versa.
     **/
    void compute()
    {
        RopeSpecBarrier.set();
        for (int i = 0; i < eTable.length; i++) {
            Node n = eTable[i];
            n.computeNewValue();
        }

        RopeSpecBarrier.set();
        for (int i = 0; i < hTable.length; i++) {
            Node n = hTable[i];
            n.computeNewValue();
        }

    }
    void __rvp__compute()
    {
    }



    /**
     * Override the toString method to print out the values of the e and h nodes.
     * @return a string contain the values of the e and h nodes.
     **/
    public String toString()
    {
        StringBuffer retval = new StringBuffer();

        for (int i = 0; i < eTable.length; i++) {
            Node n = eTable[i];
            retval.append("E: " + n + "\n");
        }
        for (int i = 0; i < hTable.length; i++) {
            Node n = hTable[i];
            retval.append("H: " + n + "\n");
        }

        return retval.toString();
    }

}
