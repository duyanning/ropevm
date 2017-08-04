#include "std.h" // precompile std.h

using namespace std;

struct Vertex {
    string from_class_name;
    string from_object_addr;
    string to_class_name;
    string to_object_addr;
};

// bool
// operator==(const Vertex& v1, const Vertex& v2)
// {
//     if (v1.class_name != v2.class_name) return false;
//     if (v1.object_addr != v2.object_addr) return false;
//     return true;
// }

struct Edge {
    // Vertex source;
    // Vertex target;
    size_t from;
    size_t to;
    int weight;
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

    while (ifs_event) {
        getline(ifs_event, line);
        cout << ":" << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        iss >> source_class_name >> source_object_addr >> arrow >> target_class_name >> target_object_addr;
        //cout << source_class_name << source_object_addr << arrow << target_class_name << target_object_addr << endl;


        // Vertex v1;
        // v1.class_name = source_class_name;
        // v1.object_addr = source_object_addr;
        // register_vertex(v1);

        // Vertex v2;
        // v2.class_name = target_class_name;
        // v2.object_addr = target_object_addr;
        // register_vertex(v2);

        Vertex v;
        v.from_class_name = source_class_name;
        v.from_object_addr = source_object_addr;
        v.to_class_name = target_class_name;
        v.to_object_addr = target_object_addr;
        register_vertex(v);


        // Edge edge;
        // edge.source = v1;
        // edge.target = v2;
        // edges.push_back(edge);
    }

    // find edges
    for (size_t i = 0; i < vertices.size(); ++i) {
        for (size_t j = i+1; j < vertices.size(); ++j) {
            Edge e;

            e.weight = 0;
            if (vertices[i].from_object_addr == vertices[j].from_object_addr) {
                e.weight += 1.0;
            }
            if (vertices[i].from_object_addr == vertices[j].to_object_addr) {
                e.weight += 1.0;
            }
            if (vertices[i].to_object_addr == vertices[j].from_object_addr) {
                e.weight += 1.0;
            }
            if (vertices[i].to_object_addr == vertices[j].to_object_addr) {
                e.weight += 1.0;
            }

            // if (i== 0 and j==4) {
            //     cout << vertices[i].from_object_addr << " " << vertices[j].from_object_addr << endl;
            //     cout << vertices[i].to_object_addr << " " << vertices[j].to_object_addr << endl;
            //     cout << e.weight << endl;
            // }

            //if (e.weight != 0) {
            if (e.weight == 2) {
                e.from = i;
                e.to = j;
                edges.push_back(e);
            }
        }
    }




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
