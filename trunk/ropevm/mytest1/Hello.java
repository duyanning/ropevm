class Beast {
    public int f()
    {
        //System.out.println("Beast.f");
        return 0;
    }


}

@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Wolf extends Beast {
    public Wolf()
    {
        int x = 0;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
    }

    public void __rvp__ctor()
    {
        int x = 0;
        x = 6;
    }
    

    public int f()
    {
        int x = 0;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        return x;
    }

    public int __rvp__f()
    {
        return 6;
        //return 5;	// wrong pslice
    }

}

@ClassGroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Hello {
    public static void main(String[] args)
    {
    }
}

