#include <fmt/core.h>
#include <iostream>
#include <vector>

enum CommandType
{
  Put,
  Get,
  Unknown
};

// Functions (Perhaps this should be in a header file or something?)
std::vector<std::string> format_input(std::string inputLine);
CommandType getCommandType(std::string inputLine);

int main(int argc, char **argv)
{
  // Client Input Loop
  std::string line;
  std::vector<std::string> splitInput;
  while (std::getline(std::cin, line))
  {
    splitInput = format_input(line);
    // Debug: Print out lines
    for (size_t i = 0; i < splitInput.size(); i++)
    {
      fmt::println(fmt::runtime("Line[" + std::to_string(i) + "]: " + splitInput[i]));
    }
    // Switch for different commands
    switch (getCommandType(splitInput.at(0)))
    {
    case CommandType::Put:
      fmt::println("Do the required code for put action.");
      break;
    case CommandType::Get:
      fmt::println("Do the required code for get action.");
      break;
    default:
      fmt::println("Unkown Command!");
    }
  }
}

// This function takes a string, splits into a string vector based on spaces
std::vector<std::string> format_input(std::string inputLine)
{
  std::vector<std::string> splitLines;
  std::string currentLine;
  // Check entire line for spaces
  for (size_t i = 0; i < inputLine.length(); i++)
  {
    if (inputLine[i] == ' ')
    { // We hit a space, so input all previous characters into the vector
      splitLines.push_back(currentLine);
      currentLine.clear();
    }
    else
    { // No space, so add current character to the line
      currentLine = currentLine + inputLine[i];
    }
  }
  // Put in the last split line
  splitLines.push_back(currentLine);
  return splitLines;
}

CommandType getCommandType(std::string inputLine)
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
