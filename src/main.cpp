#include <fstream>
#include <iostream>
#include <string>

#include "TCPClient.h"

int main(){
  std::string folder = "Temp";

  std::ifstream receive("receive.txt");
  std::ofstream send("send.txt");

  std::function logger = [](const std::string& msg) {
    std::cout << msg << std::endl;
  };

  CTCPClient tcp(logger);
  if (tcp.Connect("127.0.0.1", "8765")) {
    std::string out = (
      std::string(1, static_cast<char>(2)) +
      std::string(1, static_cast<char>(128)) +
      "White Elephant Client"
    );
    std::string header = (
      std::string(3, '\0') +
      std::string(1, static_cast<char>(out.size()))
    );
    tcp.Send(header + out);
  }

  return 0;
}
