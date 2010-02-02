@java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.RUNTIME)
@java.lang.annotation.Target({ java.lang.annotation.ElementType.TYPE })
@interface Speculative {
}

class Beast {
    public void _p_slice_for_ctor()
    {
    }


    public void f()
    {
        //System.out.println("Beast.f");
    }

    public void _p_slice_for_f()
    {
    }

}

@Speculative
class Wolf extends Beast {
    public void _p_slice_for_ctor()
    {
    }

    public void f()
    {
        //System.out.println("Wolf.f");
    }

    public void _p_slice_for_f()
    {
        //System.out.println("Wolf._simplified_f");
    }

    public void g()
    {
        //System.out.println("Wolf.g");
    }


    public void _p_slice_for_g()
    {
        //System.out.println("Wolf._simplified_g");
    }

}

class Hello {
    public static void main(String[] args)
    {
        Beast b = new Wolf();
        b.f();
        Wolf w = new Wolf();
        //w.g();
        //Beast bb = new Beast();
        //bb.f();
		int x = 50;
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

