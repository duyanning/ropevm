// 将故事图(story.txt)转换为infomap可以处理的.net格式(story.net)

#include "std.h" // precompile std.h

// linklib boost_system
// linklib boost_serialization


using namespace std;

struct Op {
    intmax_t op_no;
};

struct Story {
    intmax_t story_no;
    vector<Op> ops;
};

struct Vertex {
    Story story;
};

struct Edge {
    size_t from;
    size_t to;
    double weight;
};

vector<Vertex> vertices;
vector<Edge> edges;

ifstream ifs_story("story.txt");

void read_story(Story& story)
{
    ifs_story.ignore(numeric_limits<std::streamsize>::max(), ' ');
    ifs_story >> story.story_no;
    ifs_story.ignore(numeric_limits<std::streamsize>::max(), '\n');

    ifs_story.ignore(numeric_limits<std::streamsize>::max(), ':');
    intmax_t op_count;
    ifs_story >> op_count;

    for (intmax_t i = 0; i < op_count; ++i) {
        Op op;
        ifs_story >> op.op_no;
        story.ops.push_back(op);
    }

    ifs_story.ignore(numeric_limits<std::streamsize>::max(), '\n');
}

void output_story(const Story& story)
{
    cout << story.story_no << ": ";
    for (auto f : story.ops) {
        cout << f.op_no << " "; 
    }
    cout << endl;

}

intmax_t calc_story_overlapping(const Story& a, const Story& b)
{
    // 先判断两个故事是否重叠，如果不重叠，重叠度为0
    intmax_t a_first, a_last, b_first, b_last;
    a_first = a.ops.front().op_no;
    a_last = a.ops.back().op_no;
    b_first = b.ops.front().op_no;
    b_last = b.ops.back().op_no;

    if (a_last < b_first)
        return 0;
    if (b_last < a_first)
        return 0;

    // 如果重叠，获得两个故事的起止位置，共4个点。然后对这个4个点排序，取中间两个之间的距离，作为重叠度
    vector<intmax_t> pos;
    pos.push_back(a_first);
    pos.push_back(a_last);
    pos.push_back(b_first);
    pos.push_back(b_last);
    sort(pos.begin(), pos.end());
    
    intmax_t distance = pos[2] - pos[1] + 1;
    // output_story(a);
    // output_story(b);
    // cout << "distance: " << distance << endl;

    return distance;
}

void link_vertex()
{
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            intmax_t story_overlapping = calc_story_overlapping(vertices[j].story, vertices[i].story);
            if (story_overlapping > 0) {
                Edge e;
                e.from = i;
                e.to = j;
                e.weight = story_overlapping;
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

    ofstream ofs_net("story.net");
    ofs_net << "*Vertices " << vertices.size() << endl;
    size_t i;
    for (i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        ofs_net << i+1 << " " << "\"" << v.story.story_no << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];

        ofs_net << e.from+1 << " " << e.to+1 << " " << e.weight << endl;
    }
}

// 输出graphviz的.dot文件
void output_dot()
{
    // 注意节点编号从1开始
    ofstream ofs_dot("story.gv");
    ofs_dot << "graph G {" << endl;
    for (size_t i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];
        // const Vertex& from = vertices[e.from];
        // const Vertex& to = vertices[e.to];
        ofs_dot << e.from+1 << " -- " << e.to+1;
        ofs_dot << " [";
        // // 边的颜色
        // ofs_dot << "color=";
        // switch (e.type) {
        // case EdgeType::SAME_TARGET:
        //     ofs_dot << "green";
        //     break;
        // case EdgeType::DYNAMIC_ORDER:
        //     ofs_dot << "grey";
        //     break;
        // case EdgeType::FROM_TO_FORWARD:
        //     ofs_dot << "red";
        //     break;
        // case EdgeType::FROM_TO_BACKWARD:
        //     ofs_dot << "blue";
        //     break;
        // }
        // 边的xxx？
        ofs_dot << "weight=" << e.weight;
        // 边的粗细
        ofs_dot << ", penwidth=" << e.weight;
        ofs_dot << "];" << endl;
    }
    ofs_dot << "}" << endl;
}

void input_story_txt()
{
    ifs_story.ignore(numeric_limits<std::streamsize>::max(), ':');
    intmax_t story_count;
    ifs_story >> story_count;
    cout << "story cout: " << story_count << endl;
    for (intmax_t i = 0; i < story_count; ++i) {
        Story story;
        read_story(story);
        // for (auto f : story.ops) {
        //     cout << f.op_no << " "; 
        // }
        // cout << endl;

        Vertex vertex;
        vertex.story = story;
        vertices.push_back(vertex);
    }
}

int main()
{
    input_story_txt();
    link_vertex();
    output_net();
    output_dot();

    return 0;
}
