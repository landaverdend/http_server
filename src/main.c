#include "server.h"

int main(void)
{
  Server server = server_init(10);
  server_launch(&server);

  return 0;
}
