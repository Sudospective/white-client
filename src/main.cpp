#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "TCPClient.h"

int main() {
  std::mutex mutex;

  bool connected;

  std::vector<std::string> inputVec;

  std::function logger = [](const std::string& msg) {
    //std::cout << msg << std::endl;
  };
  CTCPClient tcp(logger);

  std::cout << "Connecting..." << std::endl;

  if (tcp.Connect("127.0.0.1", "8765")) {
    connected = true;
    std::cout << "Connected." << std::endl;
    std::ofstream status("status.txt", std::ios::trunc);
    status << "connected" << std::endl;
    status.close();
    nlohmann::json j;
    j.emplace("command", 2);
    j.emplace("message", "Hello");
    tcp.Send(j.dump());
  }
  else {
    std::cout << "Unable to connect to server." << std::endl;
    std::ofstream status("status.txt", std::ios::trunc);
    status << "inactive" << std::endl;
    status.close();
  }

  std::function reader = [&]() {
    while (true) {
      if (connected) {
        char input[1024] = {};
        int readVal = tcp.Receive(input, 1024, false);
        if (readVal < 0)
          continue;
        mutex.lock();
        if (readVal == 0) {
          std::cout << "Connection lost." << std::endl;
          connected = false;
        }
        if (readVal > 0)
          inputVec.push_back(std::string(input, 1024));
        mutex.unlock();
        continue;
      }
      std::cout << "Disconnecting..." << std::endl;
      std::ofstream status("status.txt", std::ios::trunc);
      status << "inactive" << std::endl;
      status.close();
      tcp.Disconnect();
      std::cout << "Disconnected." << std::endl;
      break;
    }
  };

  std::thread thread = std::thread(reader);

  while (true) {
    mutex.lock();
    std::vector<std::string> inputs = inputVec;
    inputVec.clear();
    mutex.unlock();
    for (std::string input : inputs) {
      nlohmann::json j = nlohmann::json::parse(input.erase(input.find_first_of('\0')));
      int cmd = j["command"];
      std::cout << "Server sent command " << cmd << std::endl;
      std::ofstream output("receive.txt");
      output << j.dump() << std::endl;
      output.close();
    }

    std::fstream send("send.txt", std::ios::in);
    std::string data;
    std::getline(send, data);
    send.close();

    if (!data.empty()) {
      // make sure its actually json
      nlohmann::json j = nlohmann::json::parse(data);
      tcp.Send(j.dump());
      // gross hack to clear the file
      send.open("send.txt", std::ios::out | std::ios::trunc);
      send.close();
    }
  }

  thread.join();

  std::ofstream status("status.txt", std::ios::trunc);
  status << "inactive" << std::endl;
  status.close();

  return 0;
}
