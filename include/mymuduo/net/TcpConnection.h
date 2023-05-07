#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/*
    TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
    =>  TcpConnection 设置回调 => Channel => Poller => Channel的回调操作

*/
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop *loop, 
        const std::string &name, 
        int sockfd, 
        const InetAddress& localaddr, 
        const InetAddress& peeraddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    //  发数据
    void send(const std::string &buf);
    void send(Buffer *buf);
    //  关闭连接
    void shutdown();

    //  设置回调函数(用户自定义的函数传入)
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark){
        highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; 
    }

    //  TcpServer会调用
    void connectEstablished();  //  连接建立
    void connectDestroyed();    //  连接销毁

private:
    enum StateE{
        kDisConnected,  //  已经断开连接
        kConnecting,    //  正在连接
        kConnected,     //  已连接
        kDisConnecting, //  正在断开连接
    };
    void setState(StateE state) { state_ = state; }

    // 注册到channel上的回调函数，poller通知后会调用这些函数处理
    // 然后这些函数最后会再调用从用户那里传来的回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void sendInLoop(const std::string& message);
    void shutdownInLoop();

    EventLoop *loop_;   //  这里绝对不是baseLoop_，因为TcpConnection都是在subloop里面管理的
    const std::string name_;
    std::atomic_int state_;     //  连接状态
    bool reading_;

    //  这里和Acceptor类似，Acceptor => mainLoop    TcpConnection => subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;   // 本服务器地址
    const InetAddress peerAddr_;    // 对端地址

    ConnectionCallback connectionCallback_; //  有新连接时的回调
    MessageCallback messageCallback_;   //  有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   //  消息发送完成以后的回调
    CloseCallback closeCallback_;   //  客户端关闭连接的回调
    HighWaterMarkCallback highWaterMarkCallback_;   //  超出水位实现的回调
    size_t highWaterMark_;

    Buffer inputBuffer_;    //  读取数据的缓冲区
    Buffer oututBuffer_;    //  发送数据的缓冲区
};


#endif