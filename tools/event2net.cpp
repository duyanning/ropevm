#include "std.h" // precompile std.h

const double weight_same_target = 1; // 大于2.0就会导致源、目标对象对调的操作之间的权重设为10的情况下，分不到一组的情况
const double weight_from_to_forward = 10; // 
const double weight_from_to_backward = 10; // 
const double weight_reentry = 20; // 重入操作之间
const double weight_dynamic_order = 1;    // 动态序列中的前后关系


using namespace std;

struct Vertex {
    string from_class_name;
    string from_object_addr;
    string to_class_name;
    string to_object_addr;
    intmax_t op_no;
    intmax_t frame_no;          // 操作所在的栈桢
    intmax_t caller_frame_no;   // 操作所在栈桢的上级栈桢
};

struct Edge {
    size_t from;
    size_t to;
    double weight;
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
    string source_class_name;
    string source_object_addr;
    string arrow;
    string target_class_name;
    string target_object_addr;
    intmax_t op_no;
    intmax_t frame_no;
    intmax_t caller_frame_no;

    while (ifs_event) {
        getline(ifs_event, line);
        //cout << ":" << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        iss >> source_class_name >> source_object_addr >> arrow >> target_class_name >> target_object_addr >> op_no
            >> frame_no >> caller_frame_no;
        //cout << source_class_name << source_object_addr << arrow << target_class_name << target_object_addr << endl;


        Vertex v;
        v.from_class_name = source_class_name;
        v.from_object_addr = source_object_addr;
        v.to_class_name = target_class_name;
        v.to_object_addr = target_object_addr;
        v.op_no = op_no;
        v.frame_no = frame_no;
        v.caller_frame_no = caller_frame_no;
        register_vertex(v);

    }
}

// 输出.net文件
void output_net()
{
    // 注意调整节点编号，使之从1开始
    cout << "vertices: " << vertices.size() << endl;
    cout << "edges: " << edges.size() << endl;

    ofstream ofs_net("event.net");
    ofs_net << "*Vertices " << vertices.size() << endl;
    size_t i;
    for (i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        ofs_net << i+1 << " " << "\"" << v.from_object_addr << "->" << v.to_object_addr << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];

        ofs_net << e.from+1 << " " << e.to+1 << " " << e.weight << endl;
    }
}

/*
  将
  a->c
  ...
  b->c
  这种有相同目标的操作联系起来(双向好还是单向好？目前是双向)
*/
void link_ops_having_same_target()
{
    // 在操作之间建立连接(连接的权重、方向都要考虑)
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            Edge e;

            // 根据操作之间的距离确定权重衰减衰系数(attenuation coefficient) 
            double ac = 1;
            intmax_t op_distance = calc_op_distance(vertices[j], vertices[i]);
            ac /= op_distance;
            //cout << i << " " << j << " distance=" << op_distance << " ac=" << ac << endl;

            // 如果目标对象相同，那么就在两个操作之间建立一条连接（对同一个对象的两个操作，应由同一线程来执行）
            if (vertices[i].to_object_addr == vertices[j].to_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = weight_same_target * ac;
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

                // 检测重入
                if (vertices[i].caller_frame_no == vertices[j].frame_no) {
                    e.weight = weight_reentry;
                }

                e.from = i;
                e.to = j;
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
        edges.push_back(e);
    }
}

void build_net()
{
    link_ops_having_same_target();
    link_ops_from_to_forward();
    link_ops_from_to_backward();
    link_ops_according_to_dynamic_order();
}


int main(int argc, char* argv[])
{
    input_log();
    build_net();
    output_net();

    return 0;
}
