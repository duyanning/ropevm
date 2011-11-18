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
        return x;
    }

    public int __rvp__f(int a)
    {
        //return 6;
        return 5;	// wrong pslice
    }

}

@ClassGroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Hello {
    public static void main(String[] args)
    {
        int x = 50;
        Beast w = new Wolf();
        // core1在猜测模式下将要执行对w.f()简化版的调用，所以snapshot，等core2执行完Wolf()的完整版返回，
        // core1得到确定模式，验证正确，提交此快照，确定执行从下面一行开始
        x -= w.f(3);
        //Dog d = new Dog();
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

        //x -= w.f();

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
    }
}

