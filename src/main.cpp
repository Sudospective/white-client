#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "TCPClient.h"

struct Gift {
  std::string ID;
  std::string author;
};

int main() {
  std::mutex mutex;
  Gift gift;

  std::vector<std::string> inputVec;

  std::function logger = [](const std::string& msg) {
    std::cout << msg << std::endl;
  };
  CTCPClient tcp(logger);

  std::cout << "Connecting..." << std::endl;

  if (tcp.Connect("127.0.0.1", "8765")) {
    std::cout << "Connected." << std::endl;
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
  else {
    std::cout << "Unable to connect to server." << std::endl;
  }

  std::function reader = [&]() {
    while (true) {
      char input[1024] = {};
      int read = tcp.Receive(input, 1024, false);
      if (read < 0) {
        std::cout << "Read Error: " << read << std::endl;
        continue;
      }
      if (read == 0) {
        std::cout << "Connection lost." << std::endl;
        tcp.Disconnect();
      }
      mutex.lock();
      if (read > 0) {
        std::string inputString = std::string(input, 1024);
        inputVec.push_back(inputString);
      }
      mutex.unlock();
      break;
    }
  };

  std::thread thread = std::thread(reader);

  thread.join();

  while (true) {
    mutex.lock();
    std::vector<std::string> inputs = inputVec;
    inputVec.clear();
    mutex.unlock();
    for (std::string input : inputs) {
      int proto = static_cast<int>(input[4]) + 128;
      std::cout << "Server sent message on protocol " << proto << std::endl;
      switch (proto) {
        case 0: { // Ping
          break;
        }
        case 1: { // Ping Response
          break;
        }
        case 2: { // Hello
          std::ofstream output("receive.txt");
          output << "2" << input.erase(0, 6).erase(input.find_first_of('\0')) << std::endl;
          output.close();
          break;
        }
        case 3: {
          break;
        }
        case 9: { // we can finally use it boys
          break;
        }
      }
    }

    std::fstream send("send.txt", std::ios::in);
    std::string data;
    std::getline(send, data);
    send.close();

    if (!data.empty()) {
      int proto = std::stoi(std::string(1, data[0]));
      if (!proto)
        continue;
      switch (proto) {
        case 9: {
          std::string out = (
            std::string(1, static_cast<char>(proto)) +
            data
          );
          std::string header = (
            std::string(3, '\0') +
            std::string(1, static_cast<char>(out.size()))
          );
          tcp.Send(header + out);
          break;
        }
      }
      // gross hack to clear the file
      send.open("send.txt", std::ios::out | std::ios::trunc);
      send.close();
    }

  }


  return 0;
}
