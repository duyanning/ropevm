#include "std.h" // precompile std.h

// 将来把程序中所有参数都放到最前面
// const double ac = 1;


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

// bool
// operator==(const Vertex& v1, const Vertex& v2)
// {
//     if (v1.class_name != v2.class_name) return false;
//     if (v1.object_addr != v2.object_addr) return false;
//     return true;
// }

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

// int
// get_vertex_index(const Vertex& v)
// {
//     auto it = find(vertices.begin(), vertices.end(), v);
//     if (it == vertices.end()) {
//         throw 1;
//     }
//     return it - vertices.begin();
// }

int main(int argc, char* argv[])
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

    // 在操作之间建立连接(连接的权重、方向都要考虑)
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            Edge e;

            // 根据操作之间的距离确定权重衰减衰系数(attenuation coefficient) 
            double ac = 1;
            intmax_t op_distance = vertices[j].op_no - vertices[i].op_no;
            ac /= op_distance;
            // cout << i << " " << j << " distance=" << op_distance << " ac=" << ac << endl;

            // 如果目标对象相同，那么就在两个操作之间建立一条连接（对同一个对象的两个操作，应由同一线程来执行）
            if (vertices[i].to_object_addr == vertices[j].to_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = 2.0 * ac; // 大于2.0就会导致源、目标对象对调的操作之间的权重设为10的情况下，分不到一组的情况
                edges.push_back(e);

                e.from = j;
                e.to = i;
                e.weight = 2.0 * ac;
                edges.push_back(e);
            }
            // 如果源对象跟目标对象正好对调，那么就在两个操作之间建立一条连接(两个对象消息来往多，不宜分在不同线程中)
            else if (vertices[i].to_object_addr == vertices[j].from_object_addr
                and vertices[i].from_object_addr == vertices[j].to_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = 4.0 * ac;
                edges.push_back(e);

                e.from = j;
                e.to = i;
                e.weight = 4.0 * ac;
                edges.push_back(e);
            }

            // 如果两个操作是重入关系（源对象跟目标对象必然对调），说
            // 明耦合很强很强（前面的操作导致的方法调用还未结束，后边
            // 的操作就开始了）。这是上一种对调情况的更进一步，需要通
            // 过栈桢编号来判断。
            if (true) {
            }

            // 其实还可以有环形重入(a调b调c调a)

            // 并不是说一有回调，两个对象就必须在一组（也意味着两个操
            // 作由同一线程执行）。因为很可能这两个对象只在一时有这种
            // 紧密联系，其他时间都没有联系。（这不就是阶段吗？）

            // if (i== 0 and j==4) {
            //     cout << vertices[i].from_object_addr << " " << vertices[j].from_object_addr << endl;
            //     cout << vertices[i].to_object_addr << " " << vertices[j].to_object_addr << endl;
            //     cout << e.weight << endl;
            // }

        }
    }

    // 按照动态序在操作之间建立单向连接
    for (size_t i = 0; i < vertices.size()-1; ++i) {
        Edge e;
        e.from = i;
        e.to = i+1;
        e.weight = 0.1;
        edges.push_back(e);
    }





    // 输出.net文件，注意调整节点编号，使之从1开始
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

    return 0;
}
