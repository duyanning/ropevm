@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Tree {
    Tree(int levels)
    {
        root = new TreeNode(levels);
    }
    void __rvp__ctor(int levels)
    {
    }

    int addTree()
    {
        return root.addTree();
    }
    int __rvp__addTree()
    {
        return 0;
    }


    private TreeNode root;

    // preload
    Tree()
    {
    }
}