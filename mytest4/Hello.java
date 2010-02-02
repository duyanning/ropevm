@java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.RUNTIME)
@java.lang.annotation.Target({ java.lang.annotation.ElementType.TYPE })
@interface Speculative {
}

class Beast {
    public int f(int a)
    {
        //System.out.println("Beast.f");
        return 0;
    }

}

@Speculative
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

    public void _p_slice_for_ctor()
    {
        int x = 0;
        x = 6;
    }

    public int g()
    {
        return 11;
    }

    public int _p_slice_for_g()
    {
        //return 11;
        return 13;	// wrong pslice
    }
}

@Speculative
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

    public void _p_slice_for_ctor()
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

    public int _p_slice_for_f(int a)
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

