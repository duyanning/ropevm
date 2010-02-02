@java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.RUNTIME)
@java.lang.annotation.Target({ java.lang.annotation.ElementType.TYPE })
@interface Speculative {
}

class Data {
	public int x;
}

@Speculative
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

    public int _p_slice_for_f()
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

