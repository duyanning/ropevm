@java.lang.annotation.Retention(java.lang.annotation.RetentionPolicy.RUNTIME)
@java.lang.annotation.Target({ java.lang.annotation.ElementType.TYPE })
@interface Speculative {
}

class Beast {
    public int f()
    {
        //System.out.println("Beast.f");
        return 0;
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
    }

    public void _p_slice_for_ctor()
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

    public int _p_slice_for_f()
    {
        return 6;
        //return 5;	// wrong pslice
    }

}

class Hello {
    public static void main(String[] args)
    {
    }
}

