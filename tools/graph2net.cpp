// 将对象协作图(graph.txt)转换为infomap可以处理的.net格式(graph.net)
#include "std.h" // precompile std.h

using namespace std;

struct Vertex {
    string class_name;
    string object_addr;
};

bool
operator==(const Vertex& v1, const Vertex& v2)
{
    if (v1.class_name != v2.class_name) return false;
    if (v1.object_addr != v2.object_addr) return false;
    return true;
}

struct Edge {
    Vertex source;
    Vertex target;
};

vector<Vertex> vertices;
vector<Edge> edges;

void
register_vertex(const Vertex& v)
{
    // auto it = find_if(vertices.begin(), vertices.end(), [&](const Vertex&v) { return  v.class_name == source_class_name and v.object_addr == source_object_addr; });

    auto it = find(vertices.begin(), vertices.end(), v);
    if (it == vertices.end()) {
        vertices.push_back(v);
    }
}

int
get_vertex_index(const Vertex& v)
{
    auto it = find(vertices.begin(), vertices.end(), v);
    if (it == vertices.end()) {
        throw 1;
    }
    return it - vertices.begin();
}

int main(int argc, char* argv[])
{
    //cout << "welcome to cpps" << endl;

    ifstream ifs_graph("graph.txt");
    string line;
    string source_class_name;
    string source_object_addr;
    string arrow;
    string target_class_name;
    string target_object_addr;

    while (ifs_graph) {
        getline(ifs_graph, line);
        //cout << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        iss >> source_class_name >> source_object_addr >> arrow >> target_class_name >> target_object_addr;
        //cout << source_class_name << source_object_addr << arrow << target_class_name << target_object_addr << endl;


        Vertex v1;
        v1.class_name = source_class_name;
        v1.object_addr = source_object_addr;
        register_vertex(v1);

        Vertex v2;
        v2.class_name = target_class_name;
        v2.object_addr = target_object_addr;
        register_vertex(v2);

        Edge edge;
        edge.source = v1;
        edge.target = v2;
        edges.push_back(edge);
    }

    cout << "vertices: " << vertices.size() << endl;
    cout << "edges: " << edges.size() << endl;

    ofstream ofs_net("graph.net");
    ofs_net << "*Vertices " << vertices.size() << endl;
    size_t i;
    for (i = 0; i < vertices.size(); ++i) {
        const Vertex& v = vertices[i];
        ofs_net << i+1 << " " << "\"" << v.class_name << " " << v.object_addr << "\"" << " 1.0" << endl;
    }

    ofs_net << "*Arcs " << edges.size() << endl;
    for (i = 0; i < edges.size(); ++i) {
        const Edge& e = edges[i];
        int source_index = get_vertex_index(e.source);
        int target_index = get_vertex_index(e.target);

        ofs_net << source_index+1 << " " << target_index+1 << " 1.0" << endl;
    }

    return 0;
}
