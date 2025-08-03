#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

// Ensure struct is packed without padding to match the exact network message
// format.
#pragma pack(push, 1)
struct SystemEventMessage {
  char messageType;
  uint16_t stockLocate;
  uint16_t trackingNumber;
  uint8_t timestamp[6];
  char eventCode;
};
#pragma pack(pop)

int main() {
  std::ifstream file("test_data.bin", std::ios::binary);
  if (!file) {
    std::cerr << "Error: Could not open test_data.bin!" << std::endl;
    return 1;
  }

  std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
                           (std::istreambuf_iterator<char>()));
  file.close();

  if (buffer.size() < sizeof(SystemEventMessage)) {
    std::cerr << "Error: File size is smaller than expected!" << std::endl;
    return 1;
  }

  SystemEventMessage msg;
  std::memcpy(&msg, buffer.data(), sizeof(SystemEventMessage));

  // --- Correct for Endianness ---
  uint16_t trackingNumber = ntohs(msg.trackingNumber);

  // Manually reconstruct the 6-byte timestamp from its byte array.
  uint64_t timestamp = 0;
  for (int i = 0; i < 6; ++i) {
    timestamp = (timestamp << 8) | msg.timestamp[i];
  }

  std::cout << "--- Parsed Message ---" << std::endl;
  std::cout << "Message Type:    " << msg.messageType << std::endl;
  std::cout << "Tracking Number: " << trackingNumber << std::endl;
  std::cout << "Timestamp:       " << timestamp << std::endl;
  std::cout << "Event Code:      " << msg.eventCode << std::endl;

  return 0;
}
