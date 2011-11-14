class Data {
	public int x;
}

@GroupingPolicies(self=GroupingPolicy.NEW_GROUP)
class Dog {
    public Data data;

    public Dog()
    {
        data = new Data();
    }

    public void f(Dog d)
    {
        data.x = 50;
    }

    public int __rvp__f()
    {
        return 6;
    }

    public void print()
    {
        System.out.println(data.x);
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

