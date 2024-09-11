#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

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
    //std::cout << msg << std::endl;
  };
  CTCPClient tcp(logger);

  std::cout << "Connecting..." << std::endl;

  if (tcp.Connect("127.0.0.1", "8765")) {
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
      char input[1024] = {};
      int read = tcp.Receive(input, 1024, false);
      if (read < 0) {
        continue;
      }
      if (read == 0) {
        std::cout << "Connection lost." << std::endl;
        std::ofstream status("status.txt", std::ios::trunc);
        status << "inactive" << std::endl;
        status.close();
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
      nlohmann::json j = nlohmann::json::parse(input);
      int cmd = j["command"];
      std::cout << "Server sent command " << cmd << std::endl;
      std::ofstream output("receive.txt");
      output << j.dump() << std::endl;
      output.close();
      break;
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

  std::ofstream status("status.txt", std::ios::trunc);
  status << "inactive" << std::endl;
  status.close();

  return 0;
}
