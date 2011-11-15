@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Dog {
    public int[] data;

    public Dog()
    {
        data = new int[1];
    }

    public void f(Dog d)
    {
        for (int i = 0; i < d.data.length; ++i) {
            d.data[i] = 50 + i;
        }
    }

    public void __rvp__f(Dog d)
    {
    }

    public void print()
    {
        for (int i = 0; i < data.length; ++i) {
            System.out.println(data[i]);
        }
    }

}

class Hello {
    public static void main(String[] args)
    {
        Dog d1 = new Dog();
        Dog d2 = new Dog();
        d1.f(d2);
        d2.f(d1);
        d2.print();
    }
}

