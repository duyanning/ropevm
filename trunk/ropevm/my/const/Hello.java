class Beast {
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


    public int f(Dog d)
    {
        int x = 0;
        x++;
        x++;
        d.g();
        x++;
        x++;
        x++;
        x++;
        return x;
    }

    public int __rvp__f(Dog d)
    {
        return 6;
        //return 5;	// wrong pslice
    }

    @RopeConst
    public int g()
    {
        int y = 0;
        y++;
        y++;
        return 0;
    }

}


@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Dog extends Beast {
    public Dog()
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


    public int f(Wolf w)
    {
        int x = 0;
        x++;
        x++;
        w.g();
        x++;
        x++;
        x++;
        x++;
        return x;
    }

    public int __rvp__f(Wolf w)
    {
        return 6;
        //return 5;	// wrong pslice
    }

    @RopeConst
    public int g()
    {
        int y = 0;
        y++;
        y++;
        return 0;
    }

}


//@ClassGroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Hello {
    public static void main(String[] args)
    {


        int x = 9;
        Wolf w = new Wolf();
        Dog d = new Dog();
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
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        w.f(d);
        d.f(w);
        x -= w.f(d);
        x -= d.f(w);
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

