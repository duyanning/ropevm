// class Graph {
// 	public void init() {
		
// 	}

// 	public void adjust() {
		
// 	}
	
// }

// class Page {
// 	public void prepare() {
		
// 	}

// 	public void layout() {
		
// 	}
// }

// class Fixer {
// 	public void tune(Graph g, Page p) {
// 		g.init();
// 		p.prepare();
// 		g.adjust();
// 		p.layout();
// 	}
	
// 	public static void main(String[] args) {
// 		Fixer fixer = new Fixer();
// 		fixer.tune(new Graph(), new Page());
// 	}
// }

/*
story count: 3
phase count: 1
phase 1:
[0, 3]
in cFixer line 31 Fixer.<init>
in cFixer line 32 Fixer.tune
可见以上程序只包含1个阶段。
这1个阶段里包含了3个故事。

查看story.txt可知
story count: 3
story 1:
op count: 3
1 4 6 
story 2:
op count: 3
2 5 7 
story 3:
op count: 2
0 3 

再查看event.txt可知
cFixer 0xaf3340f8 => Fixer 0xaf334228 0 7258 0 main <init> 31
cFixer 0xaf3340f8 => Graph 0xaf334f68 1 7258 0 main <init> 32
cFixer 0xaf3340f8 => Page 0xaf335c98 2 7258 0 main <init> 32
cFixer 0xaf3340f8 => Fixer 0xaf334228 3 7258 0 main tune 32
Fixer 0xaf334228 => Graph 0xaf334f68 4 9095 7258 tune init 24
Fixer 0xaf334228 => Page 0xaf335c98 5 9095 7258 tune prepare 25
Fixer 0xaf334228 => Graph 0xaf334f68 6 9095 7258 tune adjust 26
Fixer 0xaf334228 => Page 0xaf335c98 7 9095 7258 tune layout 27

1 4 6对应的正是g的构造方法、g.init()、g.adjust()
2 5 7对应的正是p的构造方法、p.prepare()、p.layout()
的确按照对象分成了不同的故事，而两个故事位于同一个阶段。这都很符合预期。

但我现在想要识别出多个阶段，怎么搞？
*/

class Graph {
	public void init() {
		
	}

	public void adjust() {
		
	}
	
}

class Page {
	public void prepare() {
		
	}

	public void layout() {
		
	}
}

class Fixer {
	public void tune(Graph g, Page p) {
		g.init();
		p.prepare();
		g.adjust();
		p.layout();
	}
	
	public static void main(String[] args) {
		Fixer fixer = new Fixer();
		fixer.tune(new Graph(), new Page());

		Fixer fixer2 = new Fixer();
		fixer2.tune(new Graph(), new Page());
        
	}
}
/*
story count: 4
phase count: 2
phase 1:
[0, 3]
in cFixer line 105 Fixer.<init>
in cFixer line 106 Fixer.tune
phase 2:
[8, 15]
in cFixer line 108 Fixer.<init>
in Fixer line 101 Page.layout
2个阶段，符合预期。
但为什么是4个故事，而不是翻倍变成6个故事？
而且第2个阶段的结束位置有点怪，跟第一个阶段不对称。


story count: 4
story 1:
op count: 8
8 9 10 11 12 13 14 15 
story 2:
op count: 3
1 4 6 
story 3:
op count: 3
2 5 7 
story 4:
op count: 2
0 3 

4个故事好像也不对称，查看event.txt
cFixer 0xaf349128 => Fixer 0xaf349258 0 7260 0 main <init> 105
cFixer 0xaf349128 => Graph 0xaf349f98 1 7260 0 main <init> 106
cFixer 0xaf349128 => Page 0xaf34acc8 2 7260 0 main <init> 106
cFixer 0xaf349128 => Fixer 0xaf349258 3 7260 0 main tune 106
Fixer 0xaf349258 => Graph 0xaf349f98 4 9097 7260 tune init 98
Fixer 0xaf349258 => Page 0xaf34acc8 5 9097 7260 tune prepare 99
Fixer 0xaf349258 => Graph 0xaf349f98 6 9097 7260 tune adjust 100
Fixer 0xaf349258 => Page 0xaf34acc8 7 9097 7260 tune layout 101
----------------------------------------------------------------
cFixer 0xaf349128 => Fixer 0xaf34acd8 8 7260 0 main <init> 108
cFixer 0xaf349128 => Graph 0xaf34ace8 9 7260 0 main <init> 109
cFixer 0xaf349128 => Page 0xaf34acf8 10 7260 0 main <init> 109
cFixer 0xaf349128 => Fixer 0xaf34acd8 11 7260 0 main tune 109
Fixer 0xaf34acd8 => Graph 0xaf34ace8 12 9108 7260 tune init 98
Fixer 0xaf34acd8 => Page 0xaf34acf8 13 9108 7260 tune prepare 99
Fixer 0xaf34acd8 => Graph 0xaf34ace8 14 9108 7260 tune adjust 100
Fixer 0xaf34acd8 => Page 0xaf34acf8 15 9108 7260 tune layout 101

注意：横线之上部分，同上一个程序，被分为3个故事；
可横线之下部分，跟之上部分应该是一样的，却划到了同一个故事里。

可视化看一下，横线前后部分，刚好形成差不多的两疙瘩。
在继续之前需要解决一个问题：即节点的编号问题，节点即事件，在历史中是从0开始编号的，但在.net文件中却是从1开始编号的。
这种不一致给分析带来了不必要的困难。看能不能统一一下。

先看看infomap支不支持从0开始，试了一下，infomap说
Parsing directed network from file 'event.net'... Couldn't parse line 2. Should begin with node number 1.
Be sure to use zero-based node numbering if the node numbers start from zero.
第一行说必须从1开始，第二行好像又有办法。

infomap -h看到-z参数可以支持从0开始

搞定！

看dot生成的pdf，之间关系乱七八糟的，根据“阶段.docx”一文的分析，
在节点之间只保留了一种边，即两个操作具有相同的目标对象。

正好是6个故事，两个阶段，非常对称。

下面我想试一下4个对象：a、b、c、d
在第一个阶段，a、b构成一个故事，c、d构成一个故事
在第二个阶段，a、c构成一个故事，b、d构成一个故事
之前同一个故事中的操作，其目标对象都是同一个。现在我想让同一个故事中的操作，涉及多个目标对象。
这样当我把这个故事变为一个线程时，故事中的对象才会成为同一组。
但现在根据“阶段.docx”的分析，只应该把具有相同目标对象放在同一个故事中，那如何才能让目标对象不同的多个操作进入同一个故事？
在网络中，这些目标对象不同的操作之间如何才能建立上edge？
换个方向看，从线程角度来看，同属一个组的对象之间高耦合。如何才能让两个对象耦合？
嗯？到底有没有必要让目标对象不同的多个操作进入同一个故事的必要？
可能没有！只要我们能识别出阶段，从阶段中识别高耦合团体那是对象分组社区发现算法的任务。


再开个文件夹做吧，因为cmd文件中已经把类名限制死了。
这不就是main-drive那个目录下做的事情吗？
还是新开一个，简单点。各个对象具有不同的类。
就叫four吧，表示4个对象。
 */
