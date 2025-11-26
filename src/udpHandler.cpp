#include "udpHandler.hpp"
#include "Debug.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <cstring>

#ifndef _WIN32
#include <fcntl.h>
#include <errno.h>
#endif

// Move debug constant here
static constexpr bool debug = false;

UDPHandler::UDPHandler() {
#ifdef _WIN32
    socket_fd = INVALID_SOCKET;  // Initialize first
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Debug.Error("WSA Startup failed.");
        wsa_initialized = false;
        return;
    }else{
        Debug.Log("WSA Startup successful.");
        wsa_initialized = true;
    }
#else
    socket_fd = -1;  // Initialize for Unix
#endif
}


UDPHandler::~UDPHandler() {
    stop();
#ifdef _WIN32
    if (wsa_initialized) {WSACleanup();}
#endif
}


bool UDPHandler::start() {
    if constexpr (debug) Debug.Log("Starting UDP server on port " + std::to_string(serverPort));

#ifdef _WIN32
    if (!wsa_initialized) {Debug.Error("WSA not initialized. Cannot start UDP server."); return false;}
#endif
    
    // Create socket
#ifdef _WIN32
    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == INVALID_SOCKET) {
        Debug.Error("Failed to create socket. Error: " + std::to_string(WSAGetLastError()));
        return false;
    }
#else
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        Debug.Error("Failed to create socket");
        return false;
    }
#endif

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Listen on all interfaces
    server_addr.sin_port = htons(serverPort);   // Convert port to network byte order

    // Bind socket to address and port
    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
#ifdef _WIN32
        Debug.Error("Bind failed. Error: " + std::to_string(WSAGetLastError()));
        closesocket(socket_fd);
#else
        Debug.Error("Bind failed");
        close(socket_fd);
#endif
        socket_fd = -1;
        return false;
    }

    // Set socket to non-blocking mode
#ifdef _WIN32
    u_long mode = 1;  // 1 = non-blocking, 0 = blocking
    if (ioctlsocket(socket_fd, FIONBIO, &mode) != 0) {
        if constexpr (debug) Debug.Log("Warning: Failed to set socket to non-blocking mode");
    }
#else
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        if constexpr (debug) Debug.Log("Warning: Failed to set socket to non-blocking mode");
    }
#endif

    addr_len = sizeof(client_addr);
    if constexpr (debug) Debug.Log("UDP server started successfully on port " + std::to_string(serverPort));
    return true;
}


void UDPHandler::stop() {
    if constexpr (debug) Debug.Log("Stopping UDP server...");
    
    // Check if socket is valid and close it
#ifdef _WIN32
    if (socket_fd != INVALID_SOCKET) {
        // Shutdown the socket
        shutdown(socket_fd, SD_BOTH);  // SD_BOTH = shutdown both send and receive
        
        // Close the socket
        if (closesocket(socket_fd) == SOCKET_ERROR) {
            Debug.Error("Error closing socket: " + std::to_string(WSAGetLastError()));
        } else {
            if constexpr (debug) Debug.Log("Socket closed successfully");
        }
        
        socket_fd = INVALID_SOCKET;  // Mark as invalid
    }
#else
    if (socket_fd != -1) {
        // Shutdown the socket
        shutdown(socket_fd, SHUT_RDWR);  // SHUT_RDWR = shutdown both read and write
        
        // Close the socket
        if (close(socket_fd) < 0) {
            Debug.Error("Error closing socket");
        } else {
            if constexpr (debug) Debug.Log("Socket closed successfully");
        }
        
        socket_fd = -1;  // Mark as invalid
    }
#endif
    
    // Clear address structures
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    
    // Clear message queue
    clearQueue();
    
    if constexpr (debug) Debug.Log("UDP server stopped");
}

std::string UDPHandler::receiveData() {
    // First, process any incoming data from socket to queue
    processIncomingData();
    
    // Then return from queue
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_messageQueue.empty()) { return ""; }
    
    std::string message = m_messageQueue.front();
    m_messageQueue.pop();
    m_queueSize.store(m_messageQueue.size());
    
    if constexpr (debug) {
        Debug.Log("Retrieved message from queue: '" + message + "' (Queue size: " + 
                 std::to_string(m_queueSize.load()) + ")");
    }
    
    return message;
}

bool UDPHandler::hasData() {
#ifdef _WIN32
    if (socket_fd == INVALID_SOCKET) {return false;}
#else
    if (socket_fd == -1) {return false;}
#endif

    fd_set readfds;
    struct timeval timeout;
    
    // Clear the file descriptor set
    FD_ZERO(&readfds);
    // Add our socket to the set
    FD_SET(socket_fd, &readfds);
    
    // Set timeout to 0 
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    // Check if data is available for reading
#ifdef _WIN32
    int result = select(0, &readfds, NULL, NULL, &timeout);
    // On Windows, first parameter is ignored for sockets
#else
    int result = select(socket_fd + 1, &readfds, NULL, NULL, &timeout);
    // On Unix, first parameter is highest socket fd + 1
#endif
    
    if (result < 0) {
#ifdef _WIN32
        if constexpr (debug) Debug.Error("select() failed. Error: " + std::to_string(WSAGetLastError()));
#else
        if constexpr (debug) Debug.Error("select() failed");
#endif
        return false;
    } else if (result == 0) {
        // Timeout occurred
        return false;
    } else {
        // Data is available for reading
        return FD_ISSET(socket_fd, &readfds) != 0;
    }
}

void UDPHandler::sendData(const std::string& data) {
    // TODO: Implement sending data via UDP
}

// ============================================================================
// MESSAGE QUEUE METHODS
// ============================================================================

bool UDPHandler::hasQueuedMessages() {
    return m_queueSize.load() > 0;
}

void UDPHandler::processIncomingData() {
    // Read all available data from socket and queue it
    while (hasData()) {
        std::string message = receiveDataDirect();
        if (!message.empty()) {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            
            // Check queue size limit to prevent memory overflow
            if (m_messageQueue.size() >= MAX_QUEUE_SIZE) {
                // Remove oldest message to make room
                m_messageQueue.pop();
                if constexpr (debug) {
                    Debug.Log("Warning: Message queue full, dropping oldest message");
                }
            }
            
            m_messageQueue.push(message);
            m_queueSize.store(m_messageQueue.size());
            m_packetsReceived++;
            m_bytesReceived += message.length();
            
            if constexpr (debug) {
                Debug.Log("Queued message: '" + message + "' (Queue size: " + 
                         std::to_string(m_queueSize.load()) + ")");
            }
        }
    }
}

std::string UDPHandler::receiveDataDirect() {
#ifdef _WIN32
    if (socket_fd == INVALID_SOCKET) {return "";}
#else
    if (socket_fd == -1) {return "";}
#endif

    // Buffer to store incoming data
    const int BUFFER_SIZE = 4096;  // Increased buffer size
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    socklen_t local_addr_len = sizeof(client_addr);
    
    // Receive data from client
#ifdef _WIN32
    int bytes_received = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr*)&client_addr, &local_addr_len);
    
    if (bytes_received == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAEMSGSIZE) {
            // No data available (non-blocking mode) or message too large
            return "";
        } else {
            Debug.Error("Failed to receive data. Error: " + std::to_string(error));
            return "";
        }
    }
#else
    ssize_t bytes_received = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr*)&client_addr, &local_addr_len);
    
    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available (non-blocking mode)
            return "";
        } else {
            Debug.Error("Failed to receive data");
            return "";
        }
    }
#endif

    // Null-terminate the received data
    buffer[bytes_received] = '\0';
    
    // Get client IP address for logging
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    if constexpr (debug) {
        Debug.Log("Received " + std::to_string(bytes_received) + " bytes from " + 
                 std::string(client_ip) + ":" + std::to_string(client_port) + 
                 " - Data: '" + std::string(buffer) + "'");
    }
    
    return std::string(buffer);
}

size_t UDPHandler::getQueueSize() {
    return m_queueSize.load();
}

void UDPHandler::clearQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::queue<std::string> empty;
    m_messageQueue.swap(empty);  // Clear the queue efficiently
    m_queueSize.store(0);
    if constexpr (debug) {Debug.Log("Message queue cleared");}
}