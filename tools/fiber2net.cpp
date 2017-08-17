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

int main()
{
        ifs_fiber.ignore(numeric_limits<std::streamsize>::max(), ':');
        intmax_t fiber_count;
        ifs_fiber >> fiber_count;
        cout << "fiber cout: " << fiber_count << endl;
        for (intmax_t i = 0; i < fiber_count; ++i) {
            Fiber fiber;
            read_fiber(fiber);
            for (auto f : fiber.ops) {
                cout << f.op_no << " "; 
            }
            cout << endl;
        }
 

    return 0;
}
