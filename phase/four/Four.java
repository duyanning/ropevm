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
story count: 4
phase count: 1
phase 1:
[0, 11]
in cFour line 48 A.<init>
in D line 37 C.hehe

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

        // a与b关系密切，c与d关系密切
        a.touch_b(b);
        b.touch_a(a);
        c.touch_d(d);
        d.touch_c(c);

        // 在此处增加些代码，以致使前后两个部分的事件编号相差较多（增加距离，让距离衰减因子生效）

        a.touch_c(c);
        c.touch_a(a);
        b.touch_d(d);
        d.touch_b(b);

    }

}
/*
story count: 5
phase count: 1
phase 1:
[0, 19]
in cFour line 129 A.<init>
in D line 118 B.hehe


看event.pdf(节点代表操作)，4个部分结构一模一样，彼此独立。
但为什么story.txt中说是5个故事。把其中的一个部分给拆成了2个故事。

检查了一下infomap的命令行参数，发现是作为有向图处理的，如果作为无向图，就是下面的输出：

story count: 4
phase count: 1
phase 1:
[0, 19]
in cFour line 129 A.<init>
in D line 118 B.hehe

注意：变成了4个故事。

我但觉得，事件本质上是有先后关系的，还是有向图更好吧！
为什么有向图会给识别出5个故事，我觉得可能是因为这4个部分是割裂的，infomap需要随机地跳到某个部分，如果它不巧跳到了某个部分中的一个节点，而这个节点又无法到达该部分的其他节点，可能会导致将这个部分识别为多个社区。
能否这样解决？
仍然按照动态序在节点之间建立边，给较低的权值。相同对象给较高的权值。

试了一下，效果不好啊。
可能是因为将这些原本独立的部分用动态序联系起来后，导致后边的节点承担了更多的流量。

还是换成无向图吧
无向图可以产生出4个故事
但我心里是想让让产生8个故事的。前4个故事一个阶段，后4个故事一个阶段。
为什么把前后没有分开，我觉得是因为没有考虑距离衰减因子。


 */
