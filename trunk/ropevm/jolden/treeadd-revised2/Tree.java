@Speculative
class Tree {
    Tree(int levels)
    {
        root = new TreeNode(levels);
    }
    void _p_slice_for_ctor(int levels)
    {
    }

    int addTree()
    {
        return root.addTree();
    }
    int _p_slice_for_addTree()
    {
        return 0;
    }


    private TreeNode root;

    // preload
    Tree()
    {
    }
}