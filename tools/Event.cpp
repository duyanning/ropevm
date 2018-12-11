#include "std.h"

using namespace std;

#include "Op.h"

void load_event_history(EventHistory& h)
{
    ifstream ifs_event("event.txt");
    string line;

    while (ifs_event) {
        getline(ifs_event, line);
        //cout << ":" << line << endl;
        if (line == "")
            break;
        istringstream iss(line);
        Op e;
        iss >> e;
        h.push_back(e);
    }
}
