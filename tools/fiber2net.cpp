#include "std.h" // precompile std.h

using namespace std;

struct Op {
    intmax_t op_no;
};

struct Fiber {
    intmax_t fiber_no;
    vector<Op> ops;
};

struct Vertex {
    Fiber fiber;
};

struct Edge {
    size_t from;
    size_t to;
    double weight;
};

vector<Vertex> vertices;
vector<Edge> edges;

ifstream ifs_fiber("fiber.txt");

void read_fiber(Fiber& fiber)
{
    ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), ' ');
    ifs_fiber >> fiber.fiber_no;
    ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), '\n');

    ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), ':');
    intmax_t op_count;
    ifs_fiber >> op_count;

    for (intmax_t i = 0; i < op_count; ++i) {
        Op op;
        ifs_fiber >> op.op_no;
        fiber.ops.push_back(op);
    }

    ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), '\n');
}

void output_fiber(const Fiber& fiber)
{
    cout << fiber.fiber_no << ": ";
    for (auto f : fiber.ops) {
        cout << f.op_no << " "; 
    }
    cout << endl;

}

intmax_t calc_fiber_overlapping(const Fiber& a, const Fiber& b)
{
    // 先判断两个纤维是否重叠，如果不重叠，重叠度为0
    intmax_t a_first, a_last, b_first, b_last;
    a_first = a.ops.front().op_no;
    a_last = a.ops.back().op_no;
    b_first = b.ops.front().op_no;
    b_last = b.ops.back().op_no;

    if (a_last < b_first)
        return 0;
    if (b_last < a_first)
        return 0;

    // 如果重叠，获得两个纤维的起止位置，共4个点。然后对这个4个点排序，取中间两个之间的距离，作为重叠度
    vector<intmax_t> pos;
    pos.push_back(a_first);
    pos.push_back(a_last);
    pos.push_back(b_first);
    pos.push_back(b_last);
    sort(pos.begin(), pos.end());
    
    intmax_t distance = pos[2] - pos[1] + 1;
    // output_fiber(a);
    // output_fiber(b);
    // cout << "distance: " << distance << endl;

    return distance;
}

void link_vertex()
{
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            intmax_t fiber_overlapping = calc_fiber_overlapping(vertices[j].fiber, vertices[i].fiber);
            if (fiber_overlapping > 0) {
                Edge e;
                e.from = i;
                e.to = j;
                e.weight = fiber_overlapping;
                edges.push_back(e);
            }
        }
    }
}

// 输出.net文件
void output_net()
{
    // 注意调整节点编号，使之从1开始
    cout << "vertices: " << vertices.size() << endl;
    cout << "edges: " << edges.size() << endl;

    ofstream ofs_net("fiber.net");
    ofs_net << "*Vertices " << vertices.size() << endl;
    size_t i;
    for (i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        ofs_net << i+1 << " " << "\"" << v.fiber.fiber_no << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];

        ofs_net << e.from+1 << " " << e.to+1 << " " << e.weight << endl;
    }
}

void input_fiber_txt()
{
    ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), ':');
    intmax_t fiber_count;
    ifs_fiber >> fiber_count;
    cout << "fiber cout: " << fiber_count << endl;
    for (intmax_t i = 0; i < fiber_count; ++i) {
        Fiber fiber;
        read_fiber(fiber);
        // for (auto f : fiber.ops) {
        //     cout << f.op_no << " "; 
        // }
        // cout << endl;

        Vertex vertex;
        vertex.fiber = fiber;
        vertices.push_back(vertex);
    }
}

int main()
{
    input_fiber_txt();
    link_vertex();
    output_net();

    return 0;
}
