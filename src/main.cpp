#include <fmt/core.h>
#include <iostream>
#include <vector>
#include <Client.hpp>
#include <Server.hpp>

enum CommandType
{
  Put,
  Get,
  Unknown
};

// Functions (Perhaps this should be in a header file or something?)
std::vector<std::string> format_input(std::string_view inputLine);
CommandType getCommandType(std::string_view inputLine);

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
      std::vector<std::string> words;
      while (std::getline(std::cin, line))
      {
        words = format_input(line);
        // Switch for different commands
        switch (getCommandType(words[0]))
        {
        case CommandType::Put:
          testClient.handlePutFile();
          break;
        case CommandType::Get:
          testClient.handleGetFile();
          break;
        default:
          fmt::println("Unkown Command!");
        }
      }
    }
  }
}

// This function takes a string, splits into a string vector based on spaces (words)
std::vector<std::string> format_input(std::string_view inputLine)
{
  std::vector<std::string> words;
  std::string currentWord;
  // Check entire line for spaces
  for (size_t i = 0; i < inputLine.length(); i++)
  {
    if (inputLine[i] == ' ')
    { // We hit a space, so input all previous characters into the vector
      words.push_back(currentWord);
      currentWord.clear();
    }
    else
    { // No space, so add current character to the word
      currentWord = currentWord + inputLine[i];
    }
  }
  // Put in the last word
  words.push_back(currentWord);
  return words;
}

CommandType getCommandType(std::string_view inputLine)
{
  if (inputLine == "get" || inputLine == "Get")
  {
    return CommandType::Get;
  }
  if (inputLine == "put" || inputLine == "Put")
  {
    return CommandType::Put;
  }
  return CommandType::Unknown;
}
