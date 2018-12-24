// class A {
//     void touch_b(B b) {
//         b.hehe();
//     }

//     void hehe() {
        
//     }
    
// }

// class B {

//     void touch_a(A a) {
//         a.hehe();
//     }

//     void hehe() {
        
//     }
    
// }

// class C {
//     void touch_d(D d) {
//         d.hehe();
//     }

//     void hehe() {
        
//     }
    
// }

// class D {
//     void touch_c(C c) {
//         c.hehe();
//     }

//     void hehe() {
        
//     }
    
// }

// class Four {
//     public static void main(String args[]) {
//         A a = new A();
//         B b = new B();
//         C c = new C();
//         D d = new D();

//         a.touch_b(b);
//         b.touch_a(a);
//         c.touch_d(d);
//         d.touch_c(c);
//     }

// }
/*
刚开始的结果是这样
story count: 4
phase count: 2
phase 1:
[0, 7]
in cFour line 48 A.<init>
in B line 15 A.hehe
phase 2:
[1, 6]
in cFour line 49 B.<init>
in cFour line 54 B.touch_a

每个阶段的结束位置怪怪的，后来发现map-phase-to-program.cpp中的一个bug。修复后输出如下：
story count: 4
phase count: 2
phase 1:
[0, 11]
in cFour line 48 A.<init>
in D line 37 C.hehe
phase 2:
[1, 10]
in cFour line 49 B.<init>
in cFour line 56 D.touch_c

识别出来2个阶段，可这两个阶段重叠得非常多(看操作编号反映出来的起止位置就知道)。还不如搞成一个阶段。或许是infomap觉得我必须识别出社区来。

先不管了，把想象中的第二个阶段搞出来。

 */
class A {
    void touch_b(B b) {
        b.hehe();
    }

    void touch_c(C c) {
        c.hehe();
    }
    
    void hehe() {
        
    }
    
}

class B {

    void touch_a(A a) {
        a.hehe();
    }
    void touch_d(D d) {
        d.hehe();
    }

    void hehe() {
        
    }
    
}

class C {
    void touch_d(D d) {
        d.hehe();
    }
    void touch_a(A a) {
        a.hehe();
    }

    void hehe() {
        
    }
    
}

class D {
    void touch_c(C c) {
        c.hehe();
    }
    void touch_b(B b) {
        b.hehe();
    }

    void hehe() {
        
    }
    
}

class Four {
    public static void main(String args[]) {
        A a = new A();
        B b = new B();
        C c = new C();
        D d = new D();

        a.touch_b(b);
        b.touch_a(a);
        c.touch_d(d);
        d.touch_c(c);

        a.touch_c(c);
        c.touch_a(a);
        b.touch_d(d);
        d.touch_b(b);

    }

}
/*
看event.pdf(节点代表操作)，4个部分结构一模一样，彼此独立。
但为什么story.txt中说是5个故事。把其中的一个部分给拆成了2个故事。

仔细观察了一下event.net，虽然结构一模一样，但是对应的边，权重不同。








(以下暂时作废)
但为什么产生了三个阶段？
bphase count: 3
phase 1:
story count: 2
4 5 
phase 2:
story count: 2
1 2 
phase 3:
story count: 1
3 

让infomap按无向图去处理，就是1个阶段。

但我觉得应该是两个阶段。
 */
