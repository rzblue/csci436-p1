#include <fmt/core.h>
#include <iostream>
#include <vector>

// Functions (Perhaps this should be in a header file or something?)
std::vector<std::string> format_input(std::string inputLine);

int main(int argc, char **argv)
{
  // Client Input Loop
  std::string line;
  std::vector<std::string> splitInput;
  while (std::getline(std::cin, line))
  {
    splitInput = format_input(line);
    for (size_t i = 0; i < splitInput.size(); i++)
    {
      // Debug: Print out lines
      fmt::println(fmt::runtime("Line[" + std::to_string(i) + "]: " + splitInput[i]));
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
