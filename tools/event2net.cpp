#include "std.h" // precompile std.h

using namespace std;

struct Vertex {
    string from_class_name;
    string from_object_addr;
    string to_class_name;
    string to_object_addr;
    intmax_t op_no;
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

    while (ifs_event) {
        getline(ifs_event, line);
        //cout << ":" << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        iss >> source_class_name >> source_object_addr >> arrow >> target_class_name >> target_object_addr >> op_no;
        //cout << source_class_name << source_object_addr << arrow << target_class_name << target_object_addr << endl;


        Vertex v;
        v.from_class_name = source_class_name;
        v.from_object_addr = source_object_addr;
        v.to_class_name = target_class_name;
        v.to_object_addr = target_object_addr;
        v.op_no = op_no;
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
            cout << i << " " << j << " distance=" << op_distance << " ac=" << ac << endl;

            // 如果目标对象相同，那么就在两个操作之间建立一条连接
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
            // 如果源对象跟目标对象正好对调，那么就在两个操作之间建立一条连接
            else if (vertices[i].to_object_addr == vertices[j].from_object_addr
                and vertices[i].from_object_addr == vertices[j].to_object_addr) {
                e.from = i;
                e.to = j;
                e.weight = 10.0 * ac;
                edges.push_back(e);

                e.from = j;
                e.to = i;
                e.weight = 10.0 * ac;
                edges.push_back(e);
            }


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
        e.weight = 1.0;
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
        //ofs_net << i+1 << " " << "\"" << v.from_class_name << " " << v.from_object_addr << "->" << v.to_class_name << " " << v.to_object_addr << "\"" << " 1.0" << endl;
        ofs_net << i+1 << " " << "\"" << v.from_object_addr << "->" << v.to_object_addr << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];

        ofs_net << e.from+1 << " " << e.to+1 << " " << e.weight << endl;
    }

    return 0;
}
