class Beast {
    public int f(int a)
    {
        //System.out.println("Beast.f");
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

    public int g()
    {
        return 11;
    }

    public int __rvp__g()
    {
        //return 11;
        return 13;	// wrong pslice
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
/*
        while (x < 100) {
            x++;
            x--;
        }
*/
    }

    public void __rvp__ctor()
    {
        int x = 0;
        x = 6;
    }


    public int f(int a)
    {
        int x = 0;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;
        return a + x;
    }

    public int __rvp__f(int a)
    {
        //return a + 6;
        return a;	// wrong pslice
    }

}

class Hello {
    public static void main(String[] args)
    {
        int x = 50;
        Beast w = new Wolf();
        Dog d = new Dog();
        x++;
		x += d.g();
        x -= w.f(x);
        x++;
        System.out.println("x is: " + x);
    }
}

