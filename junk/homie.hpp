#include <string>

using namespace std;

namespace garage {

class HomieMsg {
public:
  string m;
  int qos;
  bool retain;
  HomieMsg(string m, int qos, bool retain);
};

} // namespace garage