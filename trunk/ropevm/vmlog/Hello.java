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
	public native void vmlog(String str);

    public Wolf()
    {
        int x = 0;
        x++;
        x++;
        x++;
        x++;
        x++;
        x++;

		vmlog("love " + x);
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
        return x;
    }

    public int _p_slice_for_f(int a)
    {
        //return 6;
        return 5;	// wrong pslice
    }

}

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

