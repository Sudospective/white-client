#include <ctime>

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

#include "TCPClient.h"

int main() {
  std::cout << "Type in server to connect to: ";
  std::string serverAddress;
  std::cin >> serverAddress;

  std::mutex mutex;

  int serverOffset = 128;
  bool connected;

  std::vector<std::string> inputVec;

  std::function logger = [](const std::string& msg) {
    //std::cout << msg << std::endl;
  };
  CTCPClient tcp(logger);

  // gross hack to clear the file
  std::ofstream fileCleaner("receive.txt", std::ios::trunc);
  fileCleaner.close();

  std::cout << "Connecting..." << std::endl;

  if (tcp.Connect(serverAddress, "9876")) {
    connected = true;
    std::cout << "Connected." << std::endl;
  }
  else {
    std::cout << "Unable to connect to server." << std::endl;
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
      tcp.Disconnect();
      std::cout << "Disconnected." << std::endl;
      break;
    }
  };

  std::thread thread = std::thread(reader);

  while (connected) {
    mutex.lock();
    std::vector<std::string> inputs = inputVec;
    inputVec.clear();
    mutex.unlock();
    for (std::string input : inputs) {
      nlohmann::json j = nlohmann::json::parse(input.erase(input.find_first_of('\0')));
      if (j == nullptr)
        continue;
      int cmd = j["command"];
      std::cout << "Server sent command " << cmd << std::endl;
      if (cmd == serverOffset + 1) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        int64_t time = ms - static_cast<int64_t>(j["data"]["timestamp"]);
        std::cout << "Ping took " << time << "ms" << std::endl;
        j["data"]["delay"] = time;
      }
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
      nlohmann::json j;
      try {
        j = nlohmann::json::parse(data);
      }
      catch (...) {
        std::cout << "Error: invalid or malformed JSON." << std::endl;
        j["command"] = -1;
        j["data"]["message"] = "Invalid JSON";
        std::ofstream output("receive.txt");
        output << j.dump() << std::endl;
        output.close();
        send.open("send.txt", std::ios::out | std::ios::trunc);
        send.close();
        continue;
      }
      if (j["command"] == 0) {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        j["data"]["timestamp"] = ms;
      }
      tcp.Send(j.dump());
      send.open("send.txt", std::ios::out | std::ios::trunc);
      send.close();
    }
  }

  thread.join();

  return 0;
}
