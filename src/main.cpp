#include <fmt/core.h>
#include <iostream>
#include <vector>
#include <Client.hpp>
#include <Server.hpp>

int main(int argc, char *argv[])
{
  // This is for testing the server and client, probably will need to be reworked/removed
  if (argc == 2)
  {
    // Things like the port number and host set here all just for testing as well.
    if (strcmp(argv[1], "server") == 0)
    {
      Server testServer(5000);
      testServer.start();
    }
    if (strcmp(argv[1], "client") == 0)
    {
      std::string test = "localhost";
      Client testClient(5000, test.c_str());
      // Client Input Loop
      std::string line;
      while (std::getline(std::cin, line))
      {
        testClient.recieveUserInput(line);
      }
    }
  }
}
