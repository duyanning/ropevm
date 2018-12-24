// 将事件历史(event.txt)转换为infomap可以处理的.net格式(event.net)
// 这不是一个简单的格式转换，而是得确定节点间边的权重(关键所在)。

#include "std.h" // precompile std.h
using namespace std;
#include "Op.h"

// linklib boost_system

const double weight_same_target = 1; // 大于2.0就会导致源、目标对象对调的操作之间的权重设为10的情况下，分不到一组的情况
const double weight_from_to_forward = 10; // 
const double weight_from_to_backward = 10; // 
const double weight_reentry = 20; // 重入操作之间
const double weight_dynamic_order = 1;    // 动态序列中的前后关系

typedef Op Vertex;

enum class EdgeType {
    //SAME_SOURCE,                // 没啥用。放在这里是为了完整性
    SAME_TARGET,                // output dependence
    FROM_TO_FORWARD,            // true data dependence
    FROM_TO_BACKWARD,           // antidependence

    DYNAMIC_ORDER               // 有用吗？有用吗？
};

struct Edge {
    size_t from;
    size_t to;
    double weight;
    EdgeType type;
};

vector<Vertex> vertices;
vector<Edge> edges;

void
register_vertex(const Vertex& v)
{
    vertices.push_back(v);
}

// 求取两个操作之间的距离
intmax_t calc_op_distance(const Vertex& a, const Vertex& b)
{
    intmax_t d = a.op_no - b.op_no;
    return d > 0 ? d : -d;
}

// 读入日志文件
void input_log()
{
    ifstream ifs_event("event.txt");
    string line;

    while (ifs_event) {
        getline(ifs_event, line);
        //cout << ":" << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        Vertex v;
        iss >> v;
        register_vertex(v);

    }
}

// 输出.net文件
void output_net()
{
    // 注意：节点编号从0开始，调用infomap的时候要加-z参数
    cout << "vertices: " << vertices.size() << endl;
    cout << "edges: " << edges.size() << endl;

    ofstream ofs_net("event.net");
    ofs_net << "*Vertices " << vertices.size() << endl;
    size_t i;
    for (i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        // ofs_net << i+1 << " " << "\"" << v.op_no << " " << v.from_object_addr << " -> " << v.to_object_addr << "\"" << " 1.0" << endl;
        ofs_net << i << " " << "\"" << OpEntry(v) << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];

        ofs_net << e.from << " " << e.to << " " << e.weight << endl;
    }
}

/*
  将
  a->c
  ...
  b->c
  这种有相同目标的操作联系起来(双向好还是单向好？目前看来是单向。why？)
*/
void link_ops_having_same_target()
{
    // 在操作之间建立连接(连接的权重、方向都要考虑)
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            Edge e;

            // 根据操作之间的距离确定权重衰减衰系数(attenuation coefficient) 
            double ac = 1;
            //intmax_t op_distance = calc_op_distance(vertices[j], vertices[i]);
            //ac /= op_distance;
            //cout << i << " " << j << " distance=" << op_distance << " ac=" << ac << endl;

            // 如果目标对象相同，那么就在两个操作之间建立一条连接（对同一个对象的两个操作，应由同一线程来执行）
            if (vertices[i].to_object_addr == vertices[j].to_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = weight_same_target * ac;
                e.type = EdgeType::SAME_TARGET;
                edges.push_back(e);

                // e.from = j;
                // e.to = i;
                // e.weight = weight_same_target * ac;
                // edges.push_back(e);
            }

        }
    }
}

/*
  在
  a->b
  b->c
  c->a
  之间建立连接（单向），客观上导致环的出现。
  此外还要检测重入
*/
void link_ops_from_to_forward()
{
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            Edge e;

            // 根据操作之间的距离确定权重衰减衰系数(attenuation coefficient) 
            double ac = 1;
            intmax_t op_distance = calc_op_distance(vertices[j], vertices[i]);
            ac /= op_distance;

            if (vertices[i].to_object_addr == vertices[j].from_object_addr) {
                e.weight = weight_from_to_forward * ac;
                //cout << "--------------" << i+1 << " " << j+1 << endl;

                // 检测重入(有问题，还需进一步考虑：回调重入，还是返回重入)
                // if (vertices[i].caller_frame_no == vertices[j].frame_no) {
                //     assert(false); // 目前的日志，根本就到不了这里
                //     e.weight = weight_reentry;
                // }

                e.from = i;
                e.to = j;
                e.type = EdgeType::FROM_TO_FORWARD;
                edges.push_back(e);

            }

            // if (i == 2+1 and j == 3+1) {
            //     cout << "***" << endl;
            // }

            

        }
    }
}


/*
  在
  c->a
  b->c 
  a->b
  之间建立连接（单向），客观上导致环的出现。
  这种环不会有重入
*/
void link_ops_from_to_backward()
{
    for (size_t i = 1; i < vertices.size(); ++i) {
        for (size_t j = 0; j < i-1; ++j) {
            Edge e;

            // 根据操作之间的距离确定权重衰减衰系数(attenuation coefficient) 
            double ac = 1;
            intmax_t op_distance = calc_op_distance(vertices[j], vertices[i]);
            ac /= op_distance;

            if (vertices[i].to_object_addr == vertices[j].from_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = weight_from_to_backward * ac;
                e.type = EdgeType::FROM_TO_BACKWARD;
                edges.push_back(e);
            }

        }
    }
}

/*
  按照动态序在操作之间建立连接（单向）
*/
void link_ops_according_to_dynamic_order()
{
    // 按照动态序在操作之间建立单向连接
    for (size_t i = 0; i < vertices.size()-1; ++i) {
        Edge e;
        e.from = i;
        e.to = i+1;
        e.weight = weight_dynamic_order;
        e.type = EdgeType::DYNAMIC_ORDER;
        edges.push_back(e);
    }
}

void build_net()
{
    link_ops_having_same_target();
    //link_ops_from_to_forward();
    //link_ops_from_to_backward();
    //link_ops_according_to_dynamic_order();
}


// 输出graphviz的.dot文件
void output_dot()
{
    // 注意节点编号从0开始
    ofstream ofs_dot("event.gv");
    ofs_dot << "graph G {" << endl;
    for (size_t i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];
        // const Vertex& from = vertices[e.from];
        // const Vertex& to = vertices[e.to];
        ofs_dot << e.from << " -- " << e.to;
        // 边的颜色
        ofs_dot << " [color=";
        switch (e.type) {
        case EdgeType::SAME_TARGET:
            ofs_dot << "green";
            break;
        case EdgeType::DYNAMIC_ORDER:
            ofs_dot << "grey";
            break;
        case EdgeType::FROM_TO_FORWARD:
            ofs_dot << "red";
            break;
        case EdgeType::FROM_TO_BACKWARD:
            ofs_dot << "blue";
            break;
        }
        // 边的xxx？
        ofs_dot << ", weight=" << e.weight;
        // 边的粗细
        ofs_dot << ", penwidth=" << e.weight;
        ofs_dot << "];" << endl;
    }
    ofs_dot << "}" << endl;
}

int main(int argc, char* argv[])
{
    input_log();
    build_net();
    output_net();
    output_dot();

    return 0;
}

// void返回值也应当考虑，如果是void返回值，联系强度应当适当调低
