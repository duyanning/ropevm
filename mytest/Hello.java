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
		//return;
///*
        int x = 9;
        Beast b = new Wolf();
        // 不等碰到对b.f()的调用，构造函数的pslice就已经返回，所以将在确定模式下碰到对b.f的调用。
        // 48 x++
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;

        x -= b.f();

        // 48 x++
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;

        System.out.println("x is: " + x);
//*/
    }
}

